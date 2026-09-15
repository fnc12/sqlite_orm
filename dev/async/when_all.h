#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::coroutine_handle, std::suspend_never
#include <cstddef>  //  std::size_t
#include <exception>  //  std::exception_ptr, std::current_exception, std::rethrow_exception, std::terminate
#include <optional>  //  std::optional
#include <utility>  //  std::move
#include <vector>  //  std::vector
#endif

#include "task.h"

namespace sqlite_orm::internal {

    struct count_down {
        std::size_t remaining;
        std::coroutine_handle<> waiter;
    };

    struct fire_and_forget {
        struct promise_type {
            fire_and_forget get_return_object() noexcept {
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

    template<class T>
    fire_and_forget
    start_one(task<T>& oneTask, count_down& countDown, std::optional<T>& result, std::exception_ptr& error) {
        try {
            result.emplace(co_await oneTask);
        } catch (...) {
            error = std::current_exception();
        }
        if (--countDown.remaining == 0) {
            countDown.waiter.resume();
        }
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Start every task, run them concurrently, return all results in order.
     *  Rethrows the first exception after all tasks have finished.
     */
    template<class T>
    task<std::vector<T>> when_all(std::vector<task<T>> tasks) {
        std::vector<std::optional<T>> results(tasks.size());
        std::vector<std::exception_ptr> errors(tasks.size());
        internal::count_down countDown{tasks.size() + 1, {}};  //  +1: held while starting
        struct joiner {
            internal::count_down& countDown;
            std::vector<task<T>>& tasks;
            std::vector<std::optional<T>>& results;
            std::vector<std::exception_ptr>& errors;

            bool await_ready() const noexcept {
                return this->tasks.empty();
            }

            bool await_suspend(std::coroutine_handle<> handle) {
                this->countDown.waiter = handle;
                for (std::size_t index = 0; index < this->tasks.size(); ++index) {
                    internal::start_one(this->tasks[index], this->countDown, this->results[index], this->errors[index]);
                }
                return --this->countDown.remaining != 0;  //  false: everything finished synchronously
            }

            void await_resume() noexcept {}
        };
        co_await joiner{countDown, tasks, results, errors};
        for (std::exception_ptr& error: errors) {
            if (error) {
                std::rethrow_exception(error);
            }
        }
        std::vector<T> output;
        output.reserve(results.size());
        for (std::optional<T>& result: results) {
            output.push_back(std::move(*result));
        }
        co_return output;
    }
}
