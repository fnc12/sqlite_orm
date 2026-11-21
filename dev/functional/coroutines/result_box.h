#pragma once

#ifdef SQLITE_ORM_CPP20_LIB_COROUTINES_SUPPORTED
#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <exception>  //  std::exception_ptr, std::current_exception, std::rethrow_exception
#include <optional>  //  std::optional
#include <utility>  //  std::forward, std::move
#endif
#endif
#endif

#ifdef SQLITE_ORM_CPP20_LIB_COROUTINES_SUPPORTED
// general-purpose, but sqlite_orm offers its coroutine types with the asynchronous storage only
#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    /**
     *  Purpose: carry the result or the exception of a computation to the coroutine that awaits it.
     *
     *  Intention: filled by the promise of `async_task`, by `async_when_all()` for each of its tasks, and by the jobs
     *  `async_io_context::async()` runs on a fiber and `async_io_pool::async()` on another thread, each for the
     *  coroutine awaiting them.
     */
    template<class T>
    struct result_box {
        std::optional<T> value;
        std::exception_ptr error;

        template<class U>
        void set(U&& newValue) {
            this->value.emplace(std::forward<U>(newValue));
        }

        /**
         *  Keep the result of `function()`, or the exception it throws.
         */
        template<class F>
        void invoke(F& function) noexcept {
            try {
                this->set(function());
            } catch (...) {
                this->error = std::current_exception();
            }
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

        /**
         *  Run `function()`, keeping the exception it throws.
         */
        template<class F>
        void invoke(F& function) noexcept {
            try {
                function();
            } catch (...) {
                this->error = std::current_exception();
            }
        }

        void take() {
            if (this->error) {
                std::rethrow_exception(this->error);
            }
        }
    };
}
#endif
#endif
