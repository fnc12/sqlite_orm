#pragma once

/** @file
 *  Purpose: be the coroutine type an asynchronous storage operation returns.
 *
 *  Intention: realizes a lazy C++20 coroutine that any coroutine can `co_await`; used by `io_context.h`, `when_all.h`,
 *  `async_storage.h` and `async_cursor.h`.
 */

#ifdef SQLITE_ORM_CPP20_COROUTINES_SUPPORTED
#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::coroutine_handle, std::suspend_always, std::noop_coroutine
#include <exception>  //  std::exception_ptr, std::current_exception, std::rethrow_exception
#include <optional>  //  std::optional
#include <stdexcept>  //  std::logic_error
#include <utility>  //  std::exchange, std::forward, std::move
#endif
#endif
#endif

#ifdef SQLITE_ORM_CPP20_COROUTINES_SUPPORTED
// general-purpose, but sqlite_orm offers its coroutine types with the asynchronous storage only
#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    /**
     *  Result or exception of a coroutine or of a fiber job.
     */
    template<class T>
    struct result_box {
        std::optional<T> value;
        std::exception_ptr error;

        template<class U>
        void set(U&& newValue) {
            this->value.emplace(std::forward<U>(newValue));
        }

        T take() {
            if (this->error) {
                std::rethrow_exception(this->error);
            }
            return std::move(*this->value);
        }
    };

    template<>
    struct result_box<void> {
        std::exception_ptr error;

        void take() {
            if (this->error) {
                std::rethrow_exception(this->error);
            }
        }
    };

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
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Lazy coroutine type returned by asynchronous storage operations. Starts
     *  when awaited; the awaiting coroutine is resumed with the value or the
     *  exception.
     */
    template<class T>
    class async_task {
      public:
        struct promise_type : internal::task_promise_base<T> {
            async_task get_return_object() {
                return async_task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            template<class U>
            void return_value(U&& value) {
                this->box.set(std::forward<U>(value));
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
            return this->handle.promise().box.take();
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

    template<>
    class async_task<void> {
      public:
        struct promise_type : internal::task_promise_base<void> {
            async_task get_return_object() {
                return async_task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            void return_void() noexcept {}
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

        void await_resume() {
            internal::require_task(this->handle);
            this->handle.promise().box.take();
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
