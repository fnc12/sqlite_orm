#pragma once

#ifdef SQLITE_ORM_CPP20_COROUTINES_SUPPORTED
#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::suspend_never
#include <exception>  //  std::terminate
#endif
#endif
#endif

#ifdef SQLITE_ORM_CPP20_COROUTINES_SUPPORTED
// general-purpose, but sqlite_orm offers its coroutine types with the asynchronous storage only
#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    /**
     *  Purpose: start a coroutine that nobody awaits.
     *
     *  Intention: realizes an eager C++20 coroutine type that discards its result and terminates on an escaping
     *  exception; `async_when_all()` starts each of its tasks with it.
     */
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
}
#endif
#endif
