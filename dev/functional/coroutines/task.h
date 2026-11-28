#pragma once

#ifdef SQLITE_ORM_CPP20_COROUTINES_SUPPORTED
#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::coroutine_handle, std::suspend_always, std::noop_coroutine
#include <exception>  //  std::current_exception
#include <stdexcept>  //  std::logic_error
#include <utility>  //  std::exchange, std::forward, std::move
#endif
#endif
#endif

#include "result_box.h"

#ifdef SQLITE_ORM_CPP20_COROUTINES_SUPPORTED
// general-purpose, but sqlite_orm offers its coroutine types with the asynchronous storage only
#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    /**
     *  Final awaiter of an async_task: transfers control to whoever awaited the async_task.
     */
    struct task_final_awaiter {
        bool await_ready() noexcept {
            return false;
        }

        template<class Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) noexcept {
            std::coroutine_handle<> continuation = handle.promise().continuation;
            return continuation ? continuation : std::noop_coroutine();
        }

        void await_resume() noexcept {}
    };

    /**
     *  Awaiting an async_task that holds no coroutine, i.e. a default-constructed or moved-from one, is an error
     *  of the caller.
     */
    inline void require_task(std::coroutine_handle<> handle) {
        if (!handle) {
            throw std::logic_error("sqlite_orm: an empty task was awaited");
        }
    }

    template<class T>
    struct task_promise_base {
        result_box<T> box;
        std::coroutine_handle<> continuation;

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        task_final_awaiter final_suspend() noexcept {
            return {};
        }

        void unhandled_exception() noexcept {
            this->box.error = std::current_exception();
        }
    };

    /**
     *  The promise of an async_task but for get_return_object(): it keeps what `co_return` hands over.
     */
    template<class T>
    struct task_promise : task_promise_base<T> {
        template<class U>
        void return_value(U&& value) {
            this->box.set(std::forward<U>(value));
        }
    };

    template<>
    struct task_promise<void> : task_promise_base<void> {
        void return_void() noexcept {}
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Purpose: represent a computation whose result arrives later.
     *
     *  Intention: realizes a lazy C++20 coroutine type that any coroutine can `co_await`; returned by the operations of
     *  `async_storage` and `async_cursor`, awaited by `async_io_context` and `async_when_all()`.
     *
     *  Starts when awaited; the awaiting coroutine is resumed with the value or the exception. A task hands its
     *  result over once: awaited again it is empty, as a moved-from one.
     */
    template<class T>
    class async_task {
      public:
        struct promise_type : internal::task_promise<T> {
            async_task get_return_object() {
                return async_task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
        };

        async_task() = default;

        explicit async_task(std::coroutine_handle<promise_type> handle_) : handle(handle_) {}

        async_task(async_task&& other) noexcept : handle(std::exchange(other.handle, {})) {}

        async_task& operator=(async_task&& other) noexcept {
            if (this != &other) {
                this->reset();
                this->handle = std::exchange(other.handle, {});
            }
            return *this;
        }

        async_task(const async_task&) = delete;
        async_task& operator=(const async_task&) = delete;

        ~async_task() {
            this->reset();
        }

        bool await_ready() const noexcept {
            return !this->handle || this->handle.done();
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept {
            this->handle.promise().continuation = continuation;
            return this->handle;
        }

        T await_resume() {
            internal::require_task(this->handle);
            internal::result_box<T> box = std::move(this->handle.promise().box);
            this->reset();
            return box.take();
        }

      private:
        void reset() {
            if (this->handle) {
                this->handle.destroy();
                this->handle = {};
            }
        }

        std::coroutine_handle<promise_type> handle;
    };
}
#endif
#endif
