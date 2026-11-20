#pragma once

/** @file
 *  Purpose: spread work over several threads, each with its own `async_io_context`.
 *
 *  Intention: a convenience on top of `io_context.h` for applications that want I/O parallelism across storages.
 */

#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <atomic>  //  std::atomic
#include <coroutine>  //  std::coroutine_handle
#include <cstddef>  //  std::size_t
#include <exception>  //  std::current_exception, std::terminate
#include <future>  //  std::promise, std::future
#include <memory>  //  std::unique_ptr, std::shared_ptr, std::make_shared
#include <stdexcept>  //  std::logic_error
#include <thread>  //  std::thread
#include <type_traits>  //  std::invoke_result_t, std::is_void_v
#include <utility>  //  std::move
#include <vector>  //  std::vector
#endif
#endif

#include "../functional/coroutines/result_box.h"
#include "io_context.h"

#ifdef SQLITE_ORM_WITH_ASYNC
SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  M:N: `size` worker threads, each running its own async_io_context. Work is
     *  spread round-robin; a coroutine that awaits pool.async(function) is resumed
     *  back on the async_io_context it was running on.
     */
    class async_io_pool {
      public:
        explicit async_io_pool(unsigned size, unsigned queueDepth = 256) {
            for (unsigned index = 0; index < size; ++index) {
                this->workers.push_back(std::make_unique<async_io_context>(queueDepth));
            }
            for (auto& worker: this->workers) {
                this->threads.emplace_back([&worker] {
                    worker->run_forever();
                });
            }
        }

        ~async_io_pool() {
            this->shutdown();
        }

        async_io_pool(const async_io_pool&) = delete;
        async_io_pool& operator=(const async_io_pool&) = delete;

        std::size_t size() const noexcept {
            return this->workers.size();
        }

        void shutdown() {
            if (this->threads.empty()) {
                return;
            }
            for (auto& worker: this->workers) {
                worker->stop();
            }
            for (auto& thread: this->threads) {
                thread.join();
            }
            this->threads.clear();
        }

        /**
         *  Awaitable from a coroutine running on any async_io_context (pool worker or an
         *  external one): run `function` on a fiber of some worker, resume the
         *  awaiting coroutine on its own async_io_context.
         */
        template<class F>
        auto async(F function) {
            using result_type = std::invoke_result_t<F&>;
            struct awaitable {
                async_io_pool* pool = nullptr;
                F function;
                internal::result_box<result_type> box;

                bool await_ready() const noexcept {
                    return false;
                }

                void await_suspend(std::coroutine_handle<> handle) {
                    async_io_context* origin = async_io_context::current();
                    if (!origin) {
                        std::terminate();  //  must be awaited from an async_io_context
                    }
                    async_io_context* worker = this->pool->pick();
                    worker->post([this, handle, origin, worker] {
                        worker->scheduler_.spawn([this, handle, origin] {
                            this->box.invoke(this->function);
                            origin->resume_here(handle);
                        });
                    });
                }

                result_type await_resume() {
                    return this->box.take();
                }
            };
            return awaitable{this, std::move(function), {}};
        }

        /**
         *  Blocking variant for plain (non-coroutine) callers, on any thread that does not run an async_io_context:
         *  such a thread, a worker of this pool included, would wait for work that only it can do.
         *  Code running there uses `co_await pool.async(function)`.
         */
        template<class F>
        auto run_blocking(F function) -> std::invoke_result_t<F&> {
            using result_type = std::invoke_result_t<F&>;
            if (async_io_context::current()) {
                throw std::logic_error(
                    "sqlite_orm async: async_io_pool::run_blocking() called on a thread running an async_io_context");
            }
            auto promise = std::make_shared<std::promise<result_type>>();
            std::future<result_type> future = promise->get_future();
            auto sharedFunction = std::make_shared<F>(std::move(function));
            async_io_context* worker = this->pick();
            worker->post([worker, sharedFunction, promise] {
                worker->scheduler_.spawn([sharedFunction, promise] {
                    try {
                        if constexpr (std::is_void_v<result_type>) {
                            (*sharedFunction)();
                            promise->set_value();
                        } else {
                            promise->set_value((*sharedFunction)());
                        }
                    } catch (...) {
                        promise->set_exception(std::current_exception());
                    }
                });
            });
            return future.get();
        }

      private:
        async_io_context* pick() {
            const std::size_t index = this->next.fetch_add(1, std::memory_order_relaxed) % this->workers.size();
            return this->workers[index].get();
        }

        std::vector<std::unique_ptr<async_io_context>> workers;
        std::vector<std::thread> threads;
        std::atomic<std::size_t> next{0};
    };
}
#endif
