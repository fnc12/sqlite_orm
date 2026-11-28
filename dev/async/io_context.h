#pragma once

#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::coroutine_handle
#include <cstddef>  //  std::size_t
#include <deque>  //  std::deque
#include <exception>  //  std::exception_ptr, std::current_exception, std::rethrow_exception
#include <functional>  //  std::function
#include <type_traits>  //  std::invoke_result_t
#include <utility>  //  std::exchange, std::move
#endif
#endif

#include "../functional/coroutines/fire_and_forget.h"
#include "../functional/coroutines/result_box.h"
#include "../functional/coroutines/task.h"
#include "scheduler.h"

#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    template<class Storage>
    class async_storage;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Purpose: run asynchronous work on one thread.
     *
     *  Intention: the event loop applications either run themselves or embed into the event loop they already have;
     *  uses `scheduler` for the fibers and their file I/O, and `async_task` for the coroutines; `async_storage` and
     *  `async_io_pool` run on it.
     *
     *  It submits file I/O to io_uring, runs synchronous calls on fibers, and resumes the coroutines awaiting them.
     *
     *  It is not a general-purpose executor, and an application needs no second event loop because of it.
     *  There are three ways to drive it:
     *
     *  1. The application has no event loop of its own: `run()`.
     *
     *         async_io_context io;
     *         io.spawn(work(storage));    //  async_task<void> work(...)
     *         io.run();                   //  returns when everything spawned has finished
     *
     *  2. The application already runs an event loop (asio, libhv, libuv, Qt, a hand-written epoll loop):
     *     embed the async_io_context into it. `native_handle()` is a descriptor that becomes readable whenever
     *     the async_io_context has something to do; the host loop watches it and calls `poll()`, which never blocks.
     *     Coroutines are then resumed on the host loop's thread, from inside `poll()`.
     *
     *         host.watch_readable(io.native_handle(), [&io] { io.poll(); });
     *         io.spawn(work(storage));
     *         io.poll();                  //  starts what was spawned
     *
     *  3. The async_io_context gets a thread of its own: `run_forever()` there, `post()` and `stop()` from any thread.
     *
     *  Coroutines (stackless) never run on a fiber stack: a fiber hands the continuation back to the async_io_context,
     *  which resumes it on the native stack.
     */
    class async_io_context {
      public:
        /**
         *  `queueDepth` is the number of entries of the io_uring submission queue. Throws std::runtime_error where
         *  `io_uring_available()` is false.
         */
        explicit async_io_context(unsigned queueDepth = 256) : scheduler_(queueDepth) {
            this->scheduler_.set_external_ready([this] {
                return !this->ready.empty() || !this->pendingRoots.empty();
            });
            //  From here on a post() from another thread wakes run() and shows on native_handle().
            this->scheduler_.enable_wake();
            this->scheduler_.poll();
        }

        async_io_context(const async_io_context&) = delete;
        async_io_context& operator=(const async_io_context&) = delete;

        /**
         *  The async_io_context whose run()/poll()/run_forever() is executing on this
         *  thread, or nullptr.
         */
        static async_io_context* current() noexcept {
            return async_io_context::current_slot();
        }

        /**
         *  A descriptor for embedding into another event loop: it is readable whenever `poll()` has work to do
         *  that arrived from outside, i.e. completed I/O or a `post()` from another thread.
         *  Level-triggered; watch it for readability only, and never read from it or close it.
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
         *  Awaitable: run a synchronous callable on a fiber. Everything inside it, such as `sqlite3_step()`, may park
         *  on I/O without blocking the thread.
         */
        template<class F>
        auto async(F function) {
            using result_type = std::invoke_result_t<F&>;
            struct awaitable {
                async_io_context* context = nullptr;
                F function;
                internal::result_box<result_type> box;
                std::size_t stackSize = 0;

                bool await_ready() const noexcept {
                    return false;
                }

                void await_suspend(std::coroutine_handle<> handle) {
                    this->context->scheduler_.spawn(
                        [this, handle] {
                            this->box.invoke(this->function);
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
         *  Register a root coroutine. It starts on the next turn of `run()`/`poll()`/`run_forever()`, never inline,
         *  so `async_io_context::current()` is set while it executes. Same-thread; when the async_io_context is embedded
         *  into another event loop, call `poll()` afterwards.
         */
        void spawn(async_task<void> rootTask) {
            ++this->live;
            this->pendingRoots.push_back(std::move(rootTask));
        }

        /**
         *  Drive everything to completion. Rethrows the first exception that escaped
         *  a root coroutine.
         */
        void run() {
            current_guard guard(this);
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
         *  Run everything that can run without waiting, then return: never blocks. For embedding into another
         *  event loop, which calls it whenever `native_handle()` is readable and after handing over new work
         *  with `spawn()`.
         *
         *  Returns true while work is in progress. Rethrows the first exception that escaped a root coroutine.
         *  A call made from inside `poll()` itself, e.g. from a coroutine it resumed, returns at once; the
         *  outer call picks up the work.
         */
        bool poll() {
            if (this->polling) {
                return true;
            }
            current_guard guard(this);
            polling_guard pollingGuard{this};
            do {
                this->drain();
                this->scheduler_.poll();
            } while (this->has_runnable());
            this->rethrow_if_failed();
            return !(this->scheduler_.is_idle() && this->live == 0);
        }

        /**
         *  Serve posted work, fibers and coroutines until stop() is called.
         */
        void run_forever() {
            current_guard guard(this);
            this->stopped = false;
            while (!this->stopped) {
                this->drain();
                this->rethrow_if_failed();
                this->scheduler_.run_one();  //  blocks on the wake operation while idle
            }
            this->drain();
        }

        /**
         *  Thread-safe: makes `run_forever()` return after its current turn.
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
        static async_io_context*& current_slot() noexcept {
            thread_local async_io_context* currentContext = nullptr;
            return currentContext;
        }

        struct current_guard {
            async_io_context* previous = nullptr;

            explicit current_guard(async_io_context* context) : previous(async_io_context::current_slot()) {
                async_io_context::current_slot() = context;
            }

            ~current_guard() {
                async_io_context::current_slot() = this->previous;
            }
        };

        struct polling_guard {
            async_io_context* context = nullptr;

            explicit polling_guard(async_io_context* context_) : context(context_) {
                this->context->polling = true;
            }

            ~polling_guard() {
                this->context->polling = false;
            }
        };

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

        bool has_runnable() const noexcept {
            return !this->ready.empty() || !this->pendingRoots.empty() || this->scheduler_.has_runnable();
        }

        internal::fire_and_forget launch(async_task<void> rootTask) {
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
                    async_task<void> rootTask = std::move(this->pendingRoots.front());
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
        std::deque<async_task<void>> pendingRoots;
        std::size_t live = 0;
        std::exception_ptr error;
        bool stopped = false;
        bool polling = false;

        friend class async_io_pool;
        template<class Storage>
        friend class internal::async_storage;
    };
}
#endif
