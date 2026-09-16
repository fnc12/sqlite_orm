#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <cstddef>  //  std::size_t
#include <cstdint>  //  std::uintptr_t
#include <cstring>  //  std::memset
#include <exception>  //  std::terminate
#include <functional>  //  std::function
#include <stdexcept>  //  std::runtime_error
#include <utility>  //  std::move
#endif
#include <sys/mman.h>  //  mmap, mprotect, munmap
#include <unistd.h>  //  sysconf

#include "context_switch.h"

namespace sqlite_orm::internal {

    /**
     *  Stackful coroutine. One OS thread, many stacks; switching is explicit and
     *  cooperative. Used so that SQLite's synchronous VFS callbacks can suspend in
     *  the middle of sqlite3_step() without blocking the thread.
     */
    class fiber {
      public:
        static constexpr std::size_t defaultStackSize = 512 * 1024;

        explicit fiber(std::function<void()> function_, std::size_t stackSize_ = defaultStackSize) :
            function(std::move(function_)) {
            const std::size_t pageSize = fiber::page_size();
            this->stackSize = (stackSize_ + pageSize - 1) / pageSize * pageSize;
            const std::size_t totalSize = this->stackSize + pageSize;  //  plus a guard page at the bottom
            void* memory = ::mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (memory == MAP_FAILED) {
                throw std::runtime_error("sqlite_orm async: mmap of a fiber stack failed");
            }
            ::mprotect(memory, pageSize, PROT_NONE);
            this->stack = memory;
            this->stackPointer = fiber::make_initial_frame(static_cast<char*>(memory) + totalSize, &fiber::entry, this);
        }

        ~fiber() {
            if (this->stack) {
                ::munmap(this->stack, this->stackSize + fiber::page_size());
            }
        }

        fiber(const fiber&) = delete;
        fiber& operator=(const fiber&) = delete;

        /**
         *  Switch from the caller (the scheduler) into this fiber. Returns when the
         *  fiber yields or finishes.
         */
        void resume() {
            fiber* previous = fiber::current_slot();
            fiber::current_slot() = this;
            sqlite_orm_switch_context(&this->callerStackPointer, this->stackPointer);
            fiber::current_slot() = previous;
        }

        /**
         *  Switch from the running fiber back to whoever resumed it.
         */
        static void yield() {
            fiber* self = fiber::current_slot();
            sqlite_orm_switch_context(&self->stackPointer, self->callerStackPointer);
        }

        /**
         *  The fiber currently executing on this thread, or nullptr when running on
         *  the thread's native stack.
         */
        static fiber* current() noexcept {
            return fiber::current_slot();
        }

        bool is_done() const noexcept {
            return this->done;
        }

      private:
        static fiber*& current_slot() noexcept {
            thread_local fiber* currentFiber = nullptr;
            return currentFiber;
        }

        static std::size_t page_size() {
            static const std::size_t pageSize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
            return pageSize;
        }

        /**
         *  Build the initial frame that sqlite_orm_switch_context will "pop" on the first resume.
         */
        static void* make_initial_frame(void* stackTop, void (*entry)(void*), void* argument) {
            auto top = reinterpret_cast<std::uintptr_t>(stackTop) & ~std::uintptr_t{15};
#if defined(__x86_64__)
            //  Frame popped by the switch: r15 r14 r13 r12 rbx rbp, then ret. After ret
            //  rsp == top, which is 16-byte aligned, so that the trampoline's `call`
            //  leaves the callee with (rsp + 8) % 16 == 0.
            auto* frame = reinterpret_cast<void**>(top) - 7;
            frame[0] = nullptr;  //  r15
            frame[1] = nullptr;  //  r14
            frame[2] = argument;  //  r13
            frame[3] = reinterpret_cast<void*>(entry);  //  r12
            frame[4] = nullptr;  //  rbx
            frame[5] = nullptr;  //  rbp
            frame[6] = reinterpret_cast<void*>(&sqlite_orm_context_trampoline);  //  return address
            return frame;
#elif defined(__aarch64__)
            auto* frame = reinterpret_cast<void**>(top - 176);
            std::memset(frame, 0, 176);
            frame[0] = reinterpret_cast<void*>(entry);  //  x19
            frame[1] = argument;  //  x20
            frame[11] = reinterpret_cast<void*>(&sqlite_orm_context_trampoline);  //  x30 (lr)
            return frame;
#endif
        }

        static void entry(void* self) noexcept {
            auto* thisFiber = static_cast<fiber*>(self);
            try {
                thisFiber->function();
            } catch (...) {
                //  A fiber body must handle its own exceptions; there is no stack to
                //  propagate them through.
                std::terminate();
            }
            thisFiber->done = true;
            for (;;) {
                fiber::yield();  //  hand control back for the last time; never returns
            }
        }

        std::function<void()> function;
        void* stack = nullptr;  //  mmap'd region including the guard page
        std::size_t stackSize = 0;  //  usable size (without the guard page)
        void* stackPointer = nullptr;  //  saved stack pointer while suspended
        void* callerStackPointer = nullptr;  //  saved stack pointer of whoever resumed us
        bool done = false;
    };
}
