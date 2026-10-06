#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <atomic>  //  std::atomic
#include <coroutine>  //  std::coroutine_handle
#include <cstddef>  //  std::size_t
#include <exception>  //  std::current_exception, std::terminate
#include <future>  //  std::promise, std::future
#include <memory>  //  std::unique_ptr, std::shared_ptr, std::make_shared
#include <thread>  //  std::thread
#include <type_traits>  //  std::invoke_result_t, std::is_void_v
#include <utility>  //  std::move
#include <vector>  //  std::vector
#endif

#include "io_context.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  M:N: `size` worker threads, each running its own io_context. Work is
     *  spread round-robin; a coroutine that awaits pool.async(function) is resumed
     *  back on the io_context it was running on.
     */
    class io_pool {
      public:
        explicit io_pool(unsigned size, unsigned queueDepth = 256) {
            for (unsigned index = 0; index < size; ++index) {
                this->workers.push_back(std::make_unique<io_context>(queueDepth));
            }
            for (auto& worker: this->workers) {
                this->threads.emplace_back([&worker] {
                    worker->run_forever();
                });
            }
        }

        ~io_pool() {
            this->shutdown();
        }

        io_pool(const io_pool&) = delete;
        io_pool& operator=(const io_pool&) = delete;

        std::size_t size() const noexcept {
            return this->workers.size();
        }

        io_context& worker(std::size_t index) noexcept {
            return *this->workers[index];
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
         *  Awaitable from a coroutine running on any io_context (pool worker or an
         *  external one): run `function` on a fiber of some worker, resume the
         *  awaiting coroutine on its own io_context.
         */
        template<class F>
        auto async(F function) {
            using result_type = std::invoke_result_t<F&>;
            struct awaitable {
                io_pool* pool;
                F function;
                internal::result_box<result_type> box;

                bool await_ready() const noexcept {
                    return false;
                }

                void await_suspend(std::coroutine_handle<> handle) {
                    io_context* origin = io_context::current();
                    if (!origin) {
                        std::terminate();  //  must be awaited from an io_context
                    }
                    io_context* worker = this->pool->pick();
                    worker->post([this, handle, origin, worker] {
                        worker->scheduler_.spawn([this, handle, origin] {
                            try {
                                if constexpr (std::is_void_v<result_type>) {
                                    this->function();
                                } else {
                                    this->box.set(this->function());
                                }
                            } catch (...) {
                                this->box.error = std::current_exception();
                            }
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
         *  Blocking variant for plain (non-coroutine) callers on any thread.
         */
        template<class F>
        auto run_blocking(F function) -> std::invoke_result_t<F&> {
            using result_type = std::invoke_result_t<F&>;
            auto promise = std::make_shared<std::promise<result_type>>();
            std::future<result_type> future = promise->get_future();
            auto sharedFunction = std::make_shared<F>(std::move(function));
            io_context* worker = this->pick();
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
        io_context* pick() {
            const std::size_t index = this->next.fetch_add(1, std::memory_order_relaxed) % this->workers.size();
            return this->workers[index].get();
        }

        std::vector<std::unique_ptr<io_context>> workers;
        std::vector<std::thread> threads;
        std::atomic<std::size_t> next{0};
    };
}
