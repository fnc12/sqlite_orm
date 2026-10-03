#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <cerrno>  //  errno, EINTR, ENOSPC
#include <cstddef>  //  std::size_t
#include <cstring>  //  std::memcpy, std::memset, std::strdup
#include <functional>  //  std::hash
#include <mutex>  //  std::mutex, std::lock_guard
#include <string_view>  //  std::string_view
#include <unordered_map>  //  std::unordered_map
#endif
#include <fcntl.h>  //  open, O_* flags
#include <sqlite3.h>
#include <sys/stat.h>  //  stat, fstat
#include <unistd.h>  //  close

#include "scheduler.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Name of the asynchronous VFS, as accepted by `connection_control::vfs_name`.
     */
    inline constexpr std::string_view async_vfs_name = "sqlite_orm_async";

    /**
     *  Counters of the asynchronous VFS, mostly for tests.
     */
    struct async_vfs_statistics {
        sqlite3_int64 asyncReads = 0;
        sqlite3_int64 asyncWrites = 0;
        sqlite3_int64 asyncSyncs = 0;
        sqlite3_int64 asyncSleeps = 0;
        sqlite3_int64 syncReads = 0;
        sqlite3_int64 syncWrites = 0;
        sqlite3_int64 syncSyncs = 0;
    };
}

/**
 *  SQLite VFS that performs file I/O through the scheduler.
 *
 *  It wraps the platform default VFS ("unix"). Locking, shared memory (WAL
 *  index) and file open/delete are delegated to the wrapped VFS unchanged.
 *  File I/O uses a descriptor of our own (one per inode, see acquire_descriptor).
 *  xRead / xWrite / xSync / xSleep are routed through the scheduler whenever they
 *  are called from inside a fiber; otherwise they fall back to the wrapped
 *  synchronous implementation, so a connection opened with this VFS is usable
 *  from plain code as well.
 *
 */
namespace sqlite_orm::internal::async_vfs {

    inline async_vfs_statistics statistics{};

    /**
     *  SQLite allocates this struct as raw memory (szOsFile bytes), so it holds
     *  only trivially constructible members.
     */
    struct async_file {
        sqlite3_file base;  //  must be first
        sqlite3_file* wrapped;  //  wrapped unix file (allocated right after this struct)
        int descriptor;  //  our own descriptor for the file, or -1
        dev_t device;  //  inode of `descriptor` (valid when descriptor >= 0)
        ino_t inode;
    };

    inline bool in_fiber(scheduler*& currentScheduler) {
        currentScheduler = scheduler::current();
        return currentScheduler && fiber::current();
    }

    struct vfs_data {
        sqlite3_vfs* wrapped;
    };

    inline sqlite3_vfs* wrapped_vfs(sqlite3_vfs* vfs) {
        return static_cast<vfs_data*>(vfs->pAppData)->wrapped;
    }

    /**
     *  Our own descriptor for the file, independent of the wrapped unix file.
     *
     *  The descriptor is not taken from the private unixFile layout (not portable,
     *  and some builds of SQLite use guarded descriptors); the file is opened a
     *  second time instead. Closing any descriptor of a file drops every POSIX lock
     *  the process holds on it, therefore one descriptor per inode is shared by all
     *  our files and closed only when the last of them goes away.
     */
    struct inode_key {
        dev_t device;
        ino_t inode;

        bool operator==(const inode_key& other) const noexcept {
            return this->device == other.device && this->inode == other.inode;
        }
    };

    struct inode_key_hash {
        std::size_t operator()(const inode_key& key) const noexcept {
            return std::hash<unsigned long long>{}(static_cast<unsigned long long>(key.inode) * 1315423911ull ^
                                                   static_cast<unsigned long long>(key.device));
        }
    };

    struct inode_entry {
        int descriptor;
        int references;
    };

    inline std::mutex& inode_mutex() {
        static std::mutex mutex;
        return mutex;
    }

    inline std::unordered_map<inode_key, inode_entry, inode_key_hash>& inode_table() {
        static std::unordered_map<inode_key, inode_entry, inode_key_hash> table;
        return table;
    }

    /**
     *  Returns a descriptor and fills `key`, or -1 (then the file is served
     *  synchronously through the wrapped VFS).
     */
    inline int acquire_descriptor(const char* fileName, int flags, inode_key& key) {
        if (!fileName) {
            return -1;
        }
        struct stat fileStat{};
        if (::stat(fileName, &fileStat) != 0) {
            return -1;  //  e.g. a temp file already unlinked by the unix VFS
        }
        key = {fileStat.st_dev, fileStat.st_ino};
        std::lock_guard<std::mutex> lock(inode_mutex());
        auto& table = inode_table();
        auto it = table.find(key);
        if (it != table.end()) {
            ++it->second.references;
            return it->second.descriptor;
        }
        int openFlags = (flags & SQLITE_OPEN_READWRITE) ? O_RDWR : O_RDONLY;
#ifdef O_CLOEXEC
        openFlags |= O_CLOEXEC;
#endif
        int descriptor = ::open(fileName, openFlags);
        if (descriptor < 0 && (flags & SQLITE_OPEN_READWRITE)) {
            descriptor = ::open(fileName, O_RDONLY | O_CLOEXEC);
        }
        if (descriptor < 0) {
            return -1;
        }
        struct stat descriptorStat{};
        if (::fstat(descriptor, &descriptorStat) != 0 || descriptorStat.st_dev != fileStat.st_dev ||
            descriptorStat.st_ino != fileStat.st_ino) {
            ::close(descriptor);
            return -1;
        }
        table.emplace(key, inode_entry{descriptor, 1});
        return descriptor;
    }

    inline void release_descriptor(const inode_key& key) {
        std::lock_guard<std::mutex> lock(inode_mutex());
        auto& table = inode_table();
        auto it = table.find(key);
        if (it == table.end()) {
            return;
        }
        if (--it->second.references == 0) {
            ::close(it->second.descriptor);
            table.erase(it);
        }
    }

    //  ------------------------------------------------------------ io methods

    inline int file_close(sqlite3_file* file) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        int rc = SQLITE_OK;
        if (asyncFile->wrapped->pMethods) {
            rc = asyncFile->wrapped->pMethods->xClose(asyncFile->wrapped);
        }
        if (asyncFile->descriptor >= 0) {
            release_descriptor(inode_key{asyncFile->device, asyncFile->inode});
            asyncFile->descriptor = -1;
        }
        return rc;
    }

    inline int file_read(sqlite3_file* file, void* buffer, int length, sqlite3_int64 offset) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        scheduler* currentScheduler = nullptr;
        if (!in_fiber(currentScheduler) || asyncFile->descriptor < 0) {
            ++statistics.syncReads;
            if (currentScheduler) {
                currentScheduler->note_synchronous_fallback();
            }
            return asyncFile->wrapped->pMethods->xRead(asyncFile->wrapped, buffer, length, offset);
        }
        ++statistics.asyncReads;
        auto* output = static_cast<char*>(buffer);
        int got = 0;
        while (got < length) {
            const std::int32_t result = currentScheduler->read(asyncFile->descriptor,
                                                               output + got,
                                                               static_cast<unsigned>(length - got),
                                                               offset + got);
            if (result < 0) {
                if (result == -EINTR) {
                    continue;
                }
                return SQLITE_IOERR_READ;
            }
            if (result == 0) {
                break;
            }
            got += result;
        }
        if (got < length) {
            std::memset(output + got, 0, static_cast<std::size_t>(length - got));
            return SQLITE_IOERR_SHORT_READ;
        }
        return SQLITE_OK;
    }

    inline int file_write(sqlite3_file* file, const void* buffer, int length, sqlite3_int64 offset) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        scheduler* currentScheduler = nullptr;
        if (!in_fiber(currentScheduler) || asyncFile->descriptor < 0) {
            ++statistics.syncWrites;
            if (currentScheduler) {
                currentScheduler->note_synchronous_fallback();
            }
            return asyncFile->wrapped->pMethods->xWrite(asyncFile->wrapped, buffer, length, offset);
        }
        ++statistics.asyncWrites;
        auto* input = static_cast<const char*>(buffer);
        int done = 0;
        while (done < length) {
            const std::int32_t result = currentScheduler->write(asyncFile->descriptor,
                                                                input + done,
                                                                static_cast<unsigned>(length - done),
                                                                offset + done);
            if (result < 0) {
                if (result == -EINTR) {
                    continue;
                }
                return result == -ENOSPC ? SQLITE_FULL : SQLITE_IOERR_WRITE;
            }
            if (result == 0) {
                return SQLITE_FULL;
            }
            done += result;
        }
        return SQLITE_OK;
    }

    inline int file_truncate(sqlite3_file* file, sqlite3_int64 size) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xTruncate(asyncFile->wrapped, size);
    }

    inline int file_sync(sqlite3_file* file, int flags) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        scheduler* currentScheduler = nullptr;
        if (!in_fiber(currentScheduler) || asyncFile->descriptor < 0) {
            ++statistics.syncSyncs;
            if (currentScheduler) {
                currentScheduler->note_synchronous_fallback();
            }
            return asyncFile->wrapped->pMethods->xSync(asyncFile->wrapped, flags);
        }
        ++statistics.asyncSyncs;
        const bool dataOnly = (flags & SQLITE_SYNC_DATAONLY) != 0;
        const std::int32_t result = currentScheduler->fsync(asyncFile->descriptor, dataOnly);
        return result < 0 ? SQLITE_IOERR_FSYNC : SQLITE_OK;
    }

    inline int file_size(sqlite3_file* file, sqlite3_int64* size) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xFileSize(asyncFile->wrapped, size);
    }

    inline int file_lock(sqlite3_file* file, int lockType) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xLock(asyncFile->wrapped, lockType);
    }

    inline int file_unlock(sqlite3_file* file, int lockType) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xUnlock(asyncFile->wrapped, lockType);
    }

    inline int file_check_reserved_lock(sqlite3_file* file, int* result) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xCheckReservedLock(asyncFile->wrapped, result);
    }

    inline int file_control(sqlite3_file* file, int operation, void* argument) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        if (operation == SQLITE_FCNTL_VFSNAME) {
            *static_cast<char**>(argument) = sqlite3_mprintf("%s/%s", async_vfs_name.data(), "unix");
            return SQLITE_OK;
        }
        return asyncFile->wrapped->pMethods->xFileControl(asyncFile->wrapped, operation, argument);
    }

    inline int file_sector_size(sqlite3_file* file) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xSectorSize(asyncFile->wrapped);
    }

    inline int file_device_characteristics(sqlite3_file* file) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xDeviceCharacteristics(asyncFile->wrapped);
    }

    inline int file_shm_map(sqlite3_file* file, int page, int pageSize, int extend, void volatile** result) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xShmMap(asyncFile->wrapped, page, pageSize, extend, result);
    }

    inline int file_shm_lock(sqlite3_file* file, int offset, int count, int flags) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xShmLock(asyncFile->wrapped, offset, count, flags);
    }

    inline void file_shm_barrier(sqlite3_file* file) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        asyncFile->wrapped->pMethods->xShmBarrier(asyncFile->wrapped);
    }

    inline int file_shm_unmap(sqlite3_file* file, int deleteFlag) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        return asyncFile->wrapped->pMethods->xShmUnmap(asyncFile->wrapped, deleteFlag);
    }

    /**
     *  No memory-mapped I/O: a page fault would be a hidden synchronous read.
     */
    inline int file_fetch(sqlite3_file*, sqlite3_int64, int, void** result) {
        *result = nullptr;
        return SQLITE_OK;
    }

    inline int file_unfetch(sqlite3_file*, sqlite3_int64, void*) {
        return SQLITE_OK;
    }

    inline const sqlite3_io_methods ioMethods = {
        3,
        file_close,
        file_read,
        file_write,
        file_truncate,
        file_sync,
        file_size,
        file_lock,
        file_unlock,
        file_check_reserved_lock,
        file_control,
        file_sector_size,
        file_device_characteristics,
        file_shm_map,
        file_shm_lock,
        file_shm_barrier,
        file_shm_unmap,
        file_fetch,
        file_unfetch,
    };

    //  ------------------------------------------------------------ vfs methods

    inline int vfs_open(sqlite3_vfs* vfs, sqlite3_filename fileName, sqlite3_file* file, int flags, int* outputFlags) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        asyncFile->wrapped = reinterpret_cast<sqlite3_file*>(reinterpret_cast<char*>(asyncFile) + sizeof(async_file));
        asyncFile->descriptor = -1;
        std::memset(asyncFile->wrapped, 0, static_cast<std::size_t>(wrapped->szOsFile));
        const int rc = wrapped->xOpen(wrapped, fileName, asyncFile->wrapped, flags, outputFlags);
        if (rc != SQLITE_OK) {
            return rc;
        }
        inode_key key{};
        asyncFile->descriptor = acquire_descriptor(fileName, flags, key);
        if (asyncFile->descriptor >= 0) {
            asyncFile->device = key.device;
            asyncFile->inode = key.inode;
        }
        file->pMethods = &ioMethods;
        return SQLITE_OK;
    }

    inline int vfs_delete(sqlite3_vfs* vfs, const char* fileName, int syncDirectory) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xDelete(wrapped, fileName, syncDirectory);
    }

    inline int vfs_access(sqlite3_vfs* vfs, const char* fileName, int flags, int* result) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xAccess(wrapped, fileName, flags, result);
    }

    inline int vfs_full_pathname(sqlite3_vfs* vfs, const char* fileName, int length, char* output) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xFullPathname(wrapped, fileName, length, output);
    }

    inline void* vfs_dlopen(sqlite3_vfs* vfs, const char* fileName) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xDlOpen(wrapped, fileName);
    }

    inline void vfs_dlerror(sqlite3_vfs* vfs, int length, char* output) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        wrapped->xDlError(wrapped, length, output);
    }

    inline void (*vfs_dlsym(sqlite3_vfs* vfs, void* handle, const char* symbol))(void) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xDlSym(wrapped, handle, symbol);
    }

    inline void vfs_dlclose(sqlite3_vfs* vfs, void* handle) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        wrapped->xDlClose(wrapped, handle);
    }

    inline int vfs_randomness(sqlite3_vfs* vfs, int length, char* output) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xRandomness(wrapped, length, output);
    }

    inline int vfs_sleep(sqlite3_vfs* vfs, int microseconds) {
        scheduler* currentScheduler = nullptr;
        if (in_fiber(currentScheduler)) {
            ++statistics.asyncSleeps;
            currentScheduler->sleep_microseconds(microseconds);
            return microseconds;
        }
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xSleep(wrapped, microseconds);
    }

    inline int vfs_current_time(sqlite3_vfs* vfs, double* result) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xCurrentTime(wrapped, result);
    }

    inline int vfs_get_last_error(sqlite3_vfs* vfs, int length, char* output) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xGetLastError ? wrapped->xGetLastError(wrapped, length, output) : 0;
    }

    inline int vfs_current_time_int64(sqlite3_vfs* vfs, sqlite3_int64* result) {
        sqlite3_vfs* wrapped = wrapped_vfs(vfs);
        return wrapped->xCurrentTimeInt64(wrapped, result);
    }

    /**
     *  Register the VFS under async_vfs_name, wrapping the default VFS. Idempotent.
     */
    inline int register_vfs() {
        if (sqlite3_vfs_find(async_vfs_name.data())) {
            return SQLITE_OK;
        }
        sqlite3_vfs* wrapped = sqlite3_vfs_find(nullptr);
        if (!wrapped) {
            return SQLITE_ERROR;
        }
        auto* data = new vfs_data{wrapped};
        auto* vfs = new sqlite3_vfs{};
        vfs->iVersion = 2;
        vfs->szOsFile = static_cast<int>(sizeof(async_file)) + wrapped->szOsFile;
        vfs->mxPathname = wrapped->mxPathname;
        vfs->zName = async_vfs_name.data();
        vfs->pAppData = data;
        vfs->xOpen = vfs_open;
        vfs->xDelete = vfs_delete;
        vfs->xAccess = vfs_access;
        vfs->xFullPathname = vfs_full_pathname;
        vfs->xDlOpen = vfs_dlopen;
        vfs->xDlError = vfs_dlerror;
        vfs->xDlSym = vfs_dlsym;
        vfs->xDlClose = vfs_dlclose;
        vfs->xRandomness = vfs_randomness;
        vfs->xSleep = vfs_sleep;
        vfs->xCurrentTime = vfs_current_time;
        vfs->xGetLastError = vfs_get_last_error;
        vfs->xCurrentTimeInt64 = vfs_current_time_int64;
        return sqlite3_vfs_register(vfs, 0);
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    inline async_vfs_statistics get_async_vfs_statistics() {
        return internal::async_vfs::statistics;
    }

    inline void reset_async_vfs_statistics() {
        internal::async_vfs::statistics = async_vfs_statistics{};
    }
}
