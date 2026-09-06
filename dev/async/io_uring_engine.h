#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <algorithm>  //  std::max
#include <atomic>  //  std::atomic, memory orders
#include <cerrno>  //  errno
#include <cstddef>  //  std::size_t
#include <cstdint>  //  std::uint32_t, std::uint64_t, std::int64_t
#include <cstring>  //  std::memset, std::strerror
#include <stdexcept>  //  std::runtime_error
#include <string>  //  std::string
#include <unordered_map>  //  std::unordered_map
#include <vector>  //  std::vector
#endif
#include <linux/io_uring.h>  //  io_uring ABI
#include <linux/time_types.h>  //  __kernel_timespec
#include <poll.h>  //  POLLIN
#include <sys/eventfd.h>  //  eventfd
#include <sys/mman.h>  //  mmap, munmap
#include <sys/syscall.h>  //  __NR_io_uring_setup, __NR_io_uring_enter
#include <unistd.h>  //  syscall, read, write, close

#include "io_operation.h"

namespace sqlite_orm::internal {

    /**
     *  io_uring on raw syscalls (no liburing). One submission queue and one
     *  completion queue, mmap'd from the ring descriptor. submit() only publishes
     *  the submission tail; the io_uring_enter syscall happens in reap(), so one
     *  loop iteration costs one syscall, or none when nothing was queued and the
     *  caller does not block.
     */
    class io_uring_engine {
      public:
        /**
         *  Throws std::runtime_error when the kernel refuses (ENOSYS: no io_uring,
         *  EPERM: blocked by seccomp or the io_uring_disabled sysctl).
         */
        explicit io_uring_engine(unsigned entries) {
            io_uring_params params{};
            const int ringDescriptor = static_cast<int>(::syscall(__NR_io_uring_setup, entries, &params));
            if (ringDescriptor < 0) {
                io_uring_engine::fail("io_uring_setup");
            }
            this->ringDescriptor = ringDescriptor;

            this->submissionRingSize = params.sq_off.array + params.sq_entries * sizeof(std::uint32_t);
            this->completionRingSize = params.cq_off.cqes + params.cq_entries * sizeof(io_uring_cqe);
            const bool singleMapping = (params.features & IORING_FEAT_SINGLE_MMAP) != 0;
            if (singleMapping) {
                this->submissionRingSize = this->completionRingSize =
                    std::max(this->submissionRingSize, this->completionRingSize);
            }
            this->submissionRing = ::mmap(nullptr,
                                          this->submissionRingSize,
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED | MAP_POPULATE,
                                          ringDescriptor,
                                          IORING_OFF_SQ_RING);
            if (this->submissionRing == MAP_FAILED) {
                io_uring_engine::fail("mmap of the submission ring");
            }
            if (singleMapping) {
                this->completionRing = this->submissionRing;
            } else {
                this->completionRing = ::mmap(nullptr,
                                              this->completionRingSize,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED | MAP_POPULATE,
                                              ringDescriptor,
                                              IORING_OFF_CQ_RING);
                if (this->completionRing == MAP_FAILED) {
                    io_uring_engine::fail("mmap of the completion ring");
                }
            }
            this->entriesSize = params.sq_entries * sizeof(io_uring_sqe);
            this->entries = static_cast<io_uring_sqe*>(::mmap(nullptr,
                                                              this->entriesSize,
                                                              PROT_READ | PROT_WRITE,
                                                              MAP_SHARED | MAP_POPULATE,
                                                              ringDescriptor,
                                                              IORING_OFF_SQES));
            if (this->entries == MAP_FAILED) {
                io_uring_engine::fail("mmap of the submission entries");
            }

            auto* submissionBase = static_cast<char*>(this->submissionRing);
            this->submissionHead = reinterpret_cast<std::atomic<std::uint32_t>*>(submissionBase + params.sq_off.head);
            this->submissionTail = reinterpret_cast<std::atomic<std::uint32_t>*>(submissionBase + params.sq_off.tail);
            this->submissionMask = *reinterpret_cast<std::uint32_t*>(submissionBase + params.sq_off.ring_mask);
            this->submissionEntries = *reinterpret_cast<std::uint32_t*>(submissionBase + params.sq_off.ring_entries);
            this->submissionArray = reinterpret_cast<std::uint32_t*>(submissionBase + params.sq_off.array);

            auto* completionBase = static_cast<char*>(this->completionRing);
            this->completionHead = reinterpret_cast<std::atomic<std::uint32_t>*>(completionBase + params.cq_off.head);
            this->completionTail = reinterpret_cast<std::atomic<std::uint32_t>*>(completionBase + params.cq_off.tail);
            this->completionMask = *reinterpret_cast<std::uint32_t*>(completionBase + params.cq_off.ring_mask);
            this->completionEntries = reinterpret_cast<io_uring_cqe*>(completionBase + params.cq_off.cqes);

            this->wakeDescriptor = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
            if (this->wakeDescriptor < 0) {
                io_uring_engine::fail("eventfd");
            }
            this->localTail = this->submissionTail->load(std::memory_order_relaxed);
        }

        ~io_uring_engine() {
            if (this->entries && this->entries != MAP_FAILED) {
                ::munmap(this->entries, this->entriesSize);
            }
            if (this->completionRing && this->completionRing != this->submissionRing &&
                this->completionRing != MAP_FAILED) {
                ::munmap(this->completionRing, this->completionRingSize);
            }
            if (this->submissionRing && this->submissionRing != MAP_FAILED) {
                ::munmap(this->submissionRing, this->submissionRingSize);
            }
            if (this->ringDescriptor >= 0) {
                ::close(this->ringDescriptor);
            }
            if (this->wakeDescriptor >= 0) {
                ::close(this->wakeDescriptor);
            }
        }

        io_uring_engine(const io_uring_engine&) = delete;
        io_uring_engine& operator=(const io_uring_engine&) = delete;

        /**
         *  Pollable descriptor: readable when completions are waiting. Lets an
         *  external event loop know when to call the scheduler.
         */
        int native_handle() const noexcept {
            return this->ringDescriptor;
        }

        void prepare_read(io_operation& operation, int descriptor, void* buffer, unsigned length, std::int64_t offset) {
            io_uring_sqe* entry = this->next_entry(IORING_OP_READ, descriptor, &operation);
            entry->addr = reinterpret_cast<std::uint64_t>(buffer);
            entry->len = length;
            entry->off = static_cast<std::uint64_t>(offset);
        }

        void prepare_write(io_operation& operation,
                           int descriptor,
                           const void* buffer,
                           unsigned length,
                           std::int64_t offset) {
            io_uring_sqe* entry = this->next_entry(IORING_OP_WRITE, descriptor, &operation);
            entry->addr = reinterpret_cast<std::uint64_t>(buffer);
            entry->len = length;
            entry->off = static_cast<std::uint64_t>(offset);
        }

        void prepare_fsync(io_operation& operation, int descriptor, bool dataOnly) {
            io_uring_sqe* entry = this->next_entry(IORING_OP_FSYNC, descriptor, &operation);
            entry->fsync_flags = dataOnly ? IORING_FSYNC_DATASYNC : 0;
        }

        void prepare_timeout(io_operation& operation, std::int64_t microseconds) {
            //  The timespec must stay valid until completion.
            __kernel_timespec& timeSpec = this->timeouts[&operation];
            timeSpec.tv_sec = microseconds / 1000000;
            timeSpec.tv_nsec = (microseconds % 1000000) * 1000;
            io_uring_sqe* entry = this->next_entry(IORING_OP_TIMEOUT, -1, &operation);
            entry->addr = reinterpret_cast<std::uint64_t>(&timeSpec);
            entry->len = 1;
            entry->off = 0;  //  count 0: fire on time only
        }

        /**
         *  Arm `operation` so that it completes when wake() is called from any thread.
         */
        void arm_wake(io_operation& operation) {
            std::uint64_t value;
            while (::read(this->wakeDescriptor, &value, sizeof value) > 0) {
            }
            io_uring_sqe* entry = this->next_entry(IORING_OP_POLL_ADD, this->wakeDescriptor, &operation);
            entry->poll32_events = POLLIN;
        }

        void wake() {
            const std::uint64_t one = 1;
            (void)!::write(this->wakeDescriptor, &one, sizeof one);
        }

        /**
         *  Publish queued entries; the syscall is issued in reap().
         */
        void submit() {
            this->submissionTail->store(this->localTail, std::memory_order_release);
        }

        /**
         *  Collect finished operations. Blocks when `block` is true and nothing has
         *  completed yet. Returns the number of completions appended to `completed`.
         */
        std::size_t reap(bool block, std::vector<io_operation*>& completed) {
            const unsigned toSubmit = this->localTail - this->submissionHead->load(std::memory_order_acquire);
            if (toSubmit > 0 || block) {
                unsigned flags = block ? IORING_ENTER_GETEVENTS : 0;
                //  Only wait if nothing is already there to be reaped.
                if (block && this->completionHead->load(std::memory_order_relaxed) !=
                                 this->completionTail->load(std::memory_order_acquire)) {
                    flags = 0;
                }
                const int rc = static_cast<int>(
                    ::syscall(__NR_io_uring_enter, this->ringDescriptor, toSubmit, block ? 1u : 0u, flags, nullptr, 0));
                if (rc < 0 && errno != EINTR) {
                    io_uring_engine::fail("io_uring_enter");
                }
            }
            std::size_t count = 0;
            std::uint32_t head = this->completionHead->load(std::memory_order_relaxed);
            const std::uint32_t tail = this->completionTail->load(std::memory_order_acquire);
            while (head != tail) {
                const io_uring_cqe& completion = this->completionEntries[head & this->completionMask];
                auto* operation = reinterpret_cast<io_operation*>(static_cast<std::uintptr_t>(completion.user_data));
                operation->result = completion.res == -ETIME ? 0 : completion.res;
                this->timeouts.erase(operation);
                completed.push_back(operation);
                ++head;
                ++count;
            }
            this->completionHead->store(head, std::memory_order_release);
            return count;
        }

      private:
        [[noreturn]] static void fail(const char* what) {
            const int error = errno;
            throw std::runtime_error(std::string("sqlite_orm async: io_uring: ") + what + ": " + std::strerror(error));
        }

        io_uring_sqe* next_entry(std::uint8_t opcode, int descriptor, io_operation* operation) {
            if (this->localTail - this->submissionHead->load(std::memory_order_acquire) >= this->submissionEntries) {
                //  Ring full: push what we have and let the kernel drain it.
                this->submit();
                const unsigned pending = this->localTail - this->submissionHead->load(std::memory_order_acquire);
                const int rc =
                    static_cast<int>(::syscall(__NR_io_uring_enter, this->ringDescriptor, pending, 0u, 0u, nullptr, 0));
                if (rc < 0) {
                    io_uring_engine::fail("io_uring_enter on a full ring");
                }
                if (this->localTail - this->submissionHead->load(std::memory_order_acquire) >=
                    this->submissionEntries) {
                    throw std::runtime_error("sqlite_orm async: io_uring submission queue is full");
                }
            }
            const std::uint32_t index = this->localTail & this->submissionMask;
            io_uring_sqe* entry = &this->entries[index];
            std::memset(entry, 0, sizeof *entry);
            entry->opcode = opcode;
            entry->fd = descriptor;
            entry->user_data = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(operation));
            this->submissionArray[index] = index;
            ++this->localTail;
            return entry;
        }

        int ringDescriptor = -1;
        int wakeDescriptor = -1;
        void* submissionRing = nullptr;
        void* completionRing = nullptr;
        std::size_t submissionRingSize = 0;
        std::size_t completionRingSize = 0;
        std::size_t entriesSize = 0;
        io_uring_sqe* entries = nullptr;
        std::atomic<std::uint32_t>* submissionHead = nullptr;
        std::atomic<std::uint32_t>* submissionTail = nullptr;
        std::uint32_t submissionMask = 0;
        std::uint32_t submissionEntries = 0;
        std::uint32_t* submissionArray = nullptr;
        std::atomic<std::uint32_t>* completionHead = nullptr;
        std::atomic<std::uint32_t>* completionTail = nullptr;
        std::uint32_t completionMask = 0;
        io_uring_cqe* completionEntries = nullptr;
        std::uint32_t localTail = 0;
        std::unordered_map<io_operation*, __kernel_timespec> timeouts;
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  True when an io_uring instance can be created in this process. Creating an
     *  io_context where this is false throws.
     */
    inline bool io_uring_available() noexcept {
        try {
            internal::io_uring_engine probe(4);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
}
