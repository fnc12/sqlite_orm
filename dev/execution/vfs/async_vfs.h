#pragma once

/** @file
 *  Purpose: perform the file I/O of a connection through io_uring instead of blocking system calls.
 *
 *  Intention: realizes a VFS, SQLite's OS interface layer, registered next to the built-in ones of `builtin/vfs.h` and wrapping
 *  the default one; uses `scheduler.h`, and is selected for a storage by `make_async_storage()` through `connection_control`.
 */

#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <atomic>  //  std::atomic, std::memory_order_relaxed
#include <cerrno>  //  EINTR, ENOSPC
#include <cstddef>  //  std::size_t
#include <cstdint>  //  std::int32_t
#include <cstring>  //  std::memcpy, std::memset, std::strncmp
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#endif
#include <fcntl.h>  //  open, fcntl, O_* flags
#include <sqlite3.h>
#include <sys/stat.h>  //  stat, fstat
#include <unistd.h>  //  close
#endif

#include "../../async/scheduler.h"

#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

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
 *  It wraps the VFS that is the default one when the first asynchronous storage is made. Locking, shared memory
 *  (WAL index) and file open/delete are delegated to the wrapped VFS unchanged.
 *  xRead / xWrite / xSync / xSleep are routed through the scheduler whenever they are called from inside a fiber
 *  and the file has a descriptor to do so (see `wrapped_descriptor()`); otherwise they fall back to the wrapped
 *  synchronous implementation, so a connection opened with this VFS is usable from plain code as well, and over
 *  a default VFS other than SQLite's "unix".
 */
namespace sqlite_orm::internal::async_vfs {

    /**
     *  Incremented on every thread that runs an async_io_context.
     */
    struct atomic_statistics {
        std::atomic<sqlite3_int64> asyncReads{0};
        std::atomic<sqlite3_int64> asyncWrites{0};
        std::atomic<sqlite3_int64> asyncSyncs{0};
        std::atomic<sqlite3_int64> asyncSleeps{0};
        std::atomic<sqlite3_int64> syncReads{0};
        std::atomic<sqlite3_int64> syncWrites{0};
        std::atomic<sqlite3_int64> syncSyncs{0};
    };

    inline atomic_statistics statistics{};

    inline void count(std::atomic<sqlite3_int64>& counter) noexcept {
        counter.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     *  SQLite allocates this struct as raw memory (szOsFile bytes), so it holds
     *  only trivially constructible members.
     */
    struct async_file {
        sqlite3_file base{};  //  must be first
        sqlite3_file* wrapped = nullptr;  //  file of the wrapped VFS (allocated right after this struct)
        int descriptor = -1;  //  the wrapped file's descriptor, or -1: I/O goes through the wrapped file synchronously
        const char* fileName = nullptr;  //  as passed to xOpen, which SQLite keeps valid until xClose
        bool directorySyncPending =
            false;  //  a journal or WAL file this open created: its directory entry is not durable yet
    };

    inline bool in_fiber(scheduler*& currentScheduler) {
        currentScheduler = scheduler::current();
        return currentScheduler && fiber::current();
    }

    struct vfs_data {
        sqlite3_vfs* wrapped = nullptr;
    };

    inline sqlite3_vfs* wrapped_vfs(sqlite3_vfs* vfs) {
        return static_cast<vfs_data*>(vfs->pAppData)->wrapped;
    }

    /**
     *  True for the VFSes of SQLite's os_unix.c: "unix", "unix-excl", "unix-none", "unix-dotfile".
     */
    inline bool is_unix_vfs(const sqlite3_vfs* vfs) noexcept {
        return std::strncmp(vfs->zName, "unix", 4) == 0;
    }

    /**
     *  The descriptor the wrapped "unix" file itself reads and writes, or -1.
     *
     *  The asynchronous I/O must go through that very descriptor. A second one for the same file would be
     *  a hazard to the locks: closing any descriptor of a file drops every POSIX lock the process holds on it,
     *  those of other connections to the file included, which is why the "unix" VFS itself defers closing
     *  its descriptors while locks are held. And a second one opened in another mode could not serve the file.
     *
     *  SQLite offers no interface to ask for the descriptor, so it is read from where the files of os_unix.c
     *  have kept it in every release sqlite_orm supports (`unixFile::h`, after three pointers), and trusted only
     *  if it proves to be an open descriptor of this file in the mode the file was opened with. Anything else,
     *  like a default VFS that is not "unix" or a file without a name, is served synchronously through the
     *  wrapped file, which is always correct.
     */
    inline int wrapped_descriptor(const sqlite3_vfs* wrappedVfs,
                                  const sqlite3_file* wrappedFile,
                                  const char* fileName,
                                  int openedFlags) {
        if (!fileName || !is_unix_vfs(wrappedVfs)) {
            return -1;
        }
        int descriptor = -1;
        std::memcpy(&descriptor, reinterpret_cast<const char*>(wrappedFile) + 3 * sizeof(void*), sizeof descriptor);
        struct stat fileStat{};
        struct stat descriptorStat{};
        if (descriptor < 0 || ::stat(fileName, &fileStat) != 0 || ::fstat(descriptor, &descriptorStat) != 0 ||
            descriptorStat.st_dev != fileStat.st_dev || descriptorStat.st_ino != fileStat.st_ino) {
            return -1;
        }
        const int statusFlags = ::fcntl(descriptor, F_GETFL);
        const int expectedAccess = (openedFlags & SQLITE_OPEN_READWRITE) ? O_RDWR : O_RDONLY;
        if (statusFlags < 0 || (statusFlags & O_ACCMODE) != expectedAccess) {
            return -1;
        }
        return descriptor;
    }

    /**
     *  Make the directory entry of a newly created journal or WAL file durable, as the "unix" VFS does in its
     *  xSync: without it the file can be missing after a power loss, and with it the rollback of the transaction.
     */
    inline int sync_directory(scheduler& currentScheduler, const char* fileName) {
        std::string directory{fileName};
        const std::size_t slash = directory.find_last_of('/');
        if (slash == std::string::npos) {
            directory = ".";
        } else if (slash == 0) {
            directory = "/";
        } else {
            directory.resize(slash);
        }
        const int descriptor = ::open(directory.c_str(), O_RDONLY | O_CLOEXEC);
        if (descriptor < 0) {
            return SQLITE_OK;  //  a directory that cannot be opened is not synced, as with the "unix" VFS
        }
        const std::int32_t result = currentScheduler.fsync(descriptor, false);
        ::close(descriptor);
        return result < 0 ? SQLITE_IOERR_DIR_FSYNC : SQLITE_OK;
    }

    //  ------------------------------------------------------------ io methods

    inline int file_close(sqlite3_file* file) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        int rc = SQLITE_OK;
        if (asyncFile->wrapped->pMethods) {
            rc = asyncFile->wrapped->pMethods->xClose(asyncFile->wrapped);
        }
        asyncFile->descriptor = -1;  //  it was the wrapped file's
        return rc;
    }

    inline int file_read(sqlite3_file* file, void* buffer, int length, sqlite3_int64 offset) {
        auto* asyncFile = reinterpret_cast<async_file*>(file);
        scheduler* currentScheduler = nullptr;
        if (!in_fiber(currentScheduler) || asyncFile->descriptor < 0) {
            count(statistics.syncReads);
            if (currentScheduler) {
                ++currentScheduler->synchronousFallbacks;
            }
            return asyncFile->wrapped->pMethods->xRead(asyncFile->wrapped, buffer, length, offset);
        }
        count(statistics.asyncReads);
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
            count(statistics.syncWrites);
            if (currentScheduler) {
                ++currentScheduler->synchronousFallbacks;
            }
            return asyncFile->wrapped->pMethods->xWrite(asyncFile->wrapped, buffer, length, offset);
        }
        count(statistics.asyncWrites);
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
            count(statistics.syncSyncs);
            if (currentScheduler) {
                ++currentScheduler->synchronousFallbacks;
            }
            return asyncFile->wrapped->pMethods->xSync(asyncFile->wrapped, flags);
        }
        count(statistics.asyncSyncs);
        const bool dataOnly = (flags & SQLITE_SYNC_DATAONLY) != 0;
        if (currentScheduler->fsync(asyncFile->descriptor, dataOnly) < 0) {
            return SQLITE_IOERR_FSYNC;
        }
        if (asyncFile->directorySyncPending) {
            const int rc = sync_directory(*currentScheduler, asyncFile->fileName);
            if (rc != SQLITE_OK) {
                return rc;
            }
            asyncFile->directorySyncPending = false;
        }
        return SQLITE_OK;
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
            char* wrappedName = nullptr;
            asyncFile->wrapped->pMethods->xFileControl(asyncFile->wrapped, operation, &wrappedName);
            *static_cast<char**>(argument) =
                sqlite3_mprintf("%s/%s", async_vfs_name.data(), wrappedName ? wrappedName : "?");
            sqlite3_free(wrappedName);
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
        asyncFile->fileName = fileName;
        asyncFile->directorySyncPending = false;
        std::memset(asyncFile->wrapped, 0, static_cast<std::size_t>(wrapped->szOsFile));
        //  the wrapped VFS may open a file read-only that was asked for read-write, and says so in the output flags
        int openedFlags = flags;
        const int rc = wrapped->xOpen(wrapped, fileName, asyncFile->wrapped, flags, &openedFlags);
        if (rc != SQLITE_OK) {
            return rc;
        }
        if (outputFlags) {
            *outputFlags = openedFlags;
        }
        asyncFile->descriptor = wrapped_descriptor(wrapped, asyncFile->wrapped, fileName, openedFlags);
        constexpr int journalFiles = SQLITE_OPEN_MAIN_JOURNAL | SQLITE_OPEN_SUPER_JOURNAL | SQLITE_OPEN_WAL;
        asyncFile->directorySyncPending = (flags & SQLITE_OPEN_CREATE) && (flags & journalFiles);
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
            count(statistics.asyncSleeps);
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
     *  Register the VFS under `async_vfs_name` in SQLite's process-wide list of VFSes, not as the default one;
     *  it wraps the VFS that is the default at that moment. Done once per process, from whichever thread comes first.
     */
    inline int register_vfs() {
        static const int rc = [] {
            static vfs_data data{};
            static sqlite3_vfs vfs{};
            sqlite3_vfs* wrapped = sqlite3_vfs_find(nullptr);
            if (!wrapped) {
                return SQLITE_ERROR;
            }
            data.wrapped = wrapped;
            vfs.iVersion = 2;
            vfs.szOsFile = static_cast<int>(sizeof(async_file)) + wrapped->szOsFile;
            vfs.mxPathname = wrapped->mxPathname;
            vfs.zName = async_vfs_name.data();
            vfs.pAppData = &data;
            vfs.xOpen = vfs_open;
            vfs.xDelete = vfs_delete;
            vfs.xAccess = vfs_access;
            vfs.xFullPathname = vfs_full_pathname;
            vfs.xDlOpen = vfs_dlopen;
            vfs.xDlError = vfs_dlerror;
            vfs.xDlSym = vfs_dlsym;
            vfs.xDlClose = vfs_dlclose;
            vfs.xRandomness = vfs_randomness;
            vfs.xSleep = vfs_sleep;
            vfs.xCurrentTime = vfs_current_time;
            vfs.xGetLastError = vfs_get_last_error;
            vfs.xCurrentTimeInt64 = vfs_current_time_int64;
            return sqlite3_vfs_register(&vfs, 0);
        }();
        return rc;
    }
}

namespace sqlite_orm::internal {

    inline async_vfs_statistics get_async_vfs_statistics() {
        const async_vfs::atomic_statistics& counters = async_vfs::statistics;
        async_vfs_statistics snapshot;
        snapshot.asyncReads = counters.asyncReads.load(std::memory_order_relaxed);
        snapshot.asyncWrites = counters.asyncWrites.load(std::memory_order_relaxed);
        snapshot.asyncSyncs = counters.asyncSyncs.load(std::memory_order_relaxed);
        snapshot.asyncSleeps = counters.asyncSleeps.load(std::memory_order_relaxed);
        snapshot.syncReads = counters.syncReads.load(std::memory_order_relaxed);
        snapshot.syncWrites = counters.syncWrites.load(std::memory_order_relaxed);
        snapshot.syncSyncs = counters.syncSyncs.load(std::memory_order_relaxed);
        return snapshot;
    }

    inline void reset_async_vfs_statistics() {
        async_vfs::atomic_statistics& counters = async_vfs::statistics;
        for (std::atomic<sqlite3_int64>* counter: {&counters.asyncReads,
                                                   &counters.asyncWrites,
                                                   &counters.asyncSyncs,
                                                   &counters.asyncSleeps,
                                                   &counters.syncReads,
                                                   &counters.syncWrites,
                                                   &counters.syncSyncs}) {
            counter->store(0, std::memory_order_relaxed);
        }
    }
}
#endif
