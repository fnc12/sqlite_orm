#pragma once

#ifdef SQLITE_ORM_CPP20_LIB_COROUTINES_SUPPORTED
#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::coroutine_handle
#include <cstddef>  //  std::size_t
#include <exception>  //  std::current_exception
#include <type_traits>  //  std::is_void_v
#include <vector>  //  std::vector
#endif
#endif
#endif

#include "fire_and_forget.h"
#include "result_box.h"
#include "task.h"

#ifdef SQLITE_ORM_CPP20_LIB_COROUTINES_SUPPORTED
// general-purpose, but sqlite_orm offers its coroutine types with the asynchronous storage only
#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    struct count_down {
        std::size_t remaining = 0;
        std::coroutine_handle<> waiter;
    };

    template<class T>
    fire_and_forget start_one(async_task<T>& oneTask, count_down& countDown, result_box<T>& box) {
        try {
            if constexpr (std::is_void_v<T>) {
                co_await oneTask;
            } else {
                box.set(co_await oneTask);
            }
        } catch (...) {
            box.error = std::current_exception();
        }
        if (--countDown.remaining == 0) {
            countDown.waiter.resume();
        }
    }

    /**
     *  Awaited by `async_when_all()`: starts every task with `start(index)` and resumes when the last one has finished.
     */
    template<class Start>
    struct when_all_awaiter {
        std::size_t count = 0;
        Start start;
        count_down& countDown;

        bool await_ready() const noexcept {
            return this->count == 0;
        }

        bool await_suspend(std::coroutine_handle<> handle) {
            this->countDown.waiter = handle;
            for (std::size_t index = 0; index < this->count; ++index) {
                this->start(index);
            }
            return --this->countDown.remaining != 0;  //  false: everything finished synchronously
        }

        void await_resume() noexcept {}
    };

    template<class Start>
    when_all_awaiter(std::size_t, Start, count_down&) -> when_all_awaiter<Start>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Purpose: await several tasks concurrently.
     *
     *  Intention: a convenience on top of `async_task` for applications awaiting several tasks at once.
     *
     *  Start every task, run them concurrently, return all results in order. Rethrows the first exception after all
     *  tasks have finished.
     */
    template<class T>
        requires (!std::is_void_v<T>)
    async_task<std::vector<T>> async_when_all(std::vector<async_task<T>> tasks) {
        std::vector<internal::result_box<T>> boxes(tasks.size());
        internal::count_down countDown{tasks.size() + 1, {}};  //  +1: held while starting
        co_await internal::when_all_awaiter{tasks.size(),
                                            [&tasks, &countDown, &boxes](std::size_t index) {
                                                internal::start_one(tasks[index], countDown, boxes[index]);
                                            },
                                            countDown};
        std::vector<T> output;
        output.reserve(boxes.size());
        for (internal::result_box<T>& box: boxes) {
            output.push_back(box.take());  //  rethrows the first exception, in the order of the tasks
        }
        co_return output;
    }

    /**
     *  The same for tasks without a result.
     */
    inline async_task<void> async_when_all(std::vector<async_task<void>> tasks) {
        std::vector<internal::result_box<void>> boxes(tasks.size());
        internal::count_down countDown{tasks.size() + 1, {}};  //  +1: held while starting
        co_await internal::when_all_awaiter{tasks.size(),
                                            [&tasks, &countDown, &boxes](std::size_t index) {
                                                internal::start_one(tasks[index], countDown, boxes[index]);
                                            },
                                            countDown};
        for (internal::result_box<void>& box: boxes) {
            box.take();  //  rethrows the first exception, in the order of the tasks
        }
    }
}
#endif
#endif
