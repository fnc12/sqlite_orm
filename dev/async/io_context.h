#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::coroutine_handle, std::suspend_never
#include <cstddef>  //  std::size_t
#include <deque>  //  std::deque
#include <exception>  //  std::exception_ptr, std::current_exception, std::rethrow_exception, std::terminate
#include <functional>  //  std::function
#include <type_traits>  //  std::invoke_result_t, std::is_void_v
#include <utility>  //  std::exchange, std::move
#endif

#include "scheduler.h"
#include "task.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Event loop of the asynchronous storage: one per thread, drives io_uring,
     *  runs fibers and resumes coroutines. Coroutines (stackless) never run on a
     *  fiber stack: a fiber hands the continuation back to the io_context, which
     *  resumes it on the native stack.
     *
     *      io_context io;
     *      io.spawn([&]() -> task<void> {
     *          auto count = co_await io.async([&storage] { return storage.count<User>(); });   // runs on a fiber
     *      }());
     *      io.run();
     *
     *  Integration with another event loop: either give the io_context a thread
     *  of its own (run_forever / stop, post from other threads), or embed it:
     *  watch native_handle() for readability in the host loop and call poll().
     */
    class io_context {
      public:
        explicit io_context(unsigned queueDepth = 256) : scheduler_(queueDepth) {
            this->scheduler_.set_external_ready([this] {
                return !this->ready.empty() || !this->pendingRoots.empty();
            });
        }

        io_context(const io_context&) = delete;
        io_context& operator=(const io_context&) = delete;

        internal::scheduler& scheduler() noexcept {
            return this->scheduler_;
        }

        /**
         *  The io_context whose run()/poll()/run_forever() is executing on this
         *  thread, or nullptr.
         */
        static io_context* current() noexcept {
            return io_context::current_slot();
        }

        /**
         *  Pollable descriptor (readable when completions are waiting) for
         *  embedding into an external event loop together with poll().
         */
        int native_handle() const noexcept {
            return this->scheduler_.native_handle();
        }

        /**
         *  Thread-safe: run `function` on this context's thread at the next turn.
         */
        void post(std::function<void()> function) {
            this->scheduler_.post(std::move(function));
        }

        /**
         *  Thread-safe: resume a coroutine on this context's thread.
         */
        void resume_here(std::coroutine_handle<> handle) {
            this->scheduler_.post([this, handle] {
                this->ready.push_back(handle);
            });
        }

        /**
         *  Same-thread: resume `handle` on the next turn (never inline).
         */
        void schedule(std::coroutine_handle<> handle) {
            this->ready.push_back(handle);
        }

        /**
         *  Awaitable: run a synchronous callable on a fiber. Everything inside
         *  (sqlite3_step and friends) may park on I/O without blocking the thread.
         */
        template<class F>
        auto async(F function) {
            using result_type = std::invoke_result_t<F&>;
            struct awaitable {
                io_context* context;
                F function;
                internal::result_box<result_type> box;
                std::size_t stackSize;

                bool await_ready() const noexcept {
                    return false;
                }

                void await_suspend(std::coroutine_handle<> handle) {
                    this->context->scheduler_.spawn(
                        [this, handle] {
                            try {
                                if constexpr (std::is_void_v<result_type>) {
                                    this->function();
                                } else {
                                    this->box.set(this->function());
                                }
                            } catch (...) {
                                this->box.error = std::current_exception();
                            }
                            this->context->ready.push_back(handle);
                        },
                        this->stackSize);
                }

                result_type await_resume() {
                    return this->box.take();
                }
            };
            return awaitable{this, std::move(function), {}, this->fiberStackSize};
        }

        /**
         *  Register a root coroutine. It starts on the next turn of run()/poll()
         *  (never inline), so io_context::current() is set while it executes.
         */
        void spawn(task<void> rootTask) {
            ++this->live;
            this->pendingRoots.push_back(std::move(rootTask));
        }

        /**
         *  Drive everything to completion. Rethrows the first exception that escaped
         *  a root coroutine.
         */
        void run() {
            current_guard guard(this);
            this->scheduler_.enable_wake();  //  so that run_one() can block on cross-thread posts
            for (;;) {
                this->drain();
                this->rethrow_if_failed();
                if (this->ready.empty() && this->scheduler_.is_idle() && this->live == 0) {
                    break;
                }
                if (this->ready.empty()) {
                    this->scheduler_.run_one();  //  runs fibers, or blocks for I/O / posts
                }
            }
        }

        /**
         *  One non-blocking turn; returns true if there is still work.
         */
        bool poll() {
            current_guard guard(this);
            this->drain();
            this->scheduler_.poll();
            this->drain();
            return !(this->ready.empty() && this->scheduler_.is_idle() && this->live == 0);
        }

        /**
         *  Serve posted work, fibers and coroutines until stop() is called.
         */
        void run_forever() {
            current_guard guard(this);
            this->stopped = false;
            this->scheduler_.enable_wake();
            while (!this->stopped) {
                this->drain();
                this->rethrow_if_failed();
                this->scheduler_.run_one();  //  blocks on the wake operation while idle
            }
            this->drain();
        }

        /**
         *  Thread-safe.
         */
        void stop() {
            this->scheduler_.post([this] {
                this->stopped = true;
            });
        }

        /**
         *  Stack size of the fibers created by async().
         */
        std::size_t fiberStackSize = internal::fiber::defaultStackSize;

      private:
        static io_context*& current_slot() noexcept {
            thread_local io_context* currentContext = nullptr;
            return currentContext;
        }

        struct current_guard {
            io_context* previous;

            explicit current_guard(io_context* context) : previous(io_context::current_slot()) {
                io_context::current_slot() = context;
            }

            ~current_guard() {
                io_context::current_slot() = this->previous;
            }
        };

        struct detached {
            struct promise_type {
                detached get_return_object() noexcept {
                    return {};
                }

                std::suspend_never initial_suspend() noexcept {
                    return {};
                }

                std::suspend_never final_suspend() noexcept {
                    return {};
                }

                void return_void() noexcept {}

                void unhandled_exception() noexcept {
                    std::terminate();
                }
            };
        };

        detached launch(task<void> rootTask) {
            try {
                co_await std::move(rootTask);
            } catch (...) {
                if (!this->error) {
                    this->error = std::current_exception();
                }
            }
            --this->live;
        }

        void rethrow_if_failed() {
            if (this->error) {
                std::exception_ptr error = std::exchange(this->error, nullptr);
                std::rethrow_exception(error);
            }
        }

        void drain() {
            for (;;) {
                if (!this->pendingRoots.empty()) {
                    task<void> rootTask = std::move(this->pendingRoots.front());
                    this->pendingRoots.pop_front();
                    this->launch(std::move(rootTask));
                    continue;
                }
                if (this->ready.empty()) {
                    break;
                }
                std::coroutine_handle<> handle = this->ready.front();
                this->ready.pop_front();
                handle.resume();
            }
        }

        internal::scheduler scheduler_;
        std::deque<std::coroutine_handle<>> ready;
        std::deque<task<void>> pendingRoots;
        std::size_t live = 0;
        std::exception_ptr error;
        bool stopped = false;

        friend class io_pool;
    };
}
