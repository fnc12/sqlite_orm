#pragma once

/** @file
 *  Purpose: describe one asynchronous I/O request in flight.
 *
 *  Intention: the record shared by `io_uring_engine.h`, which submits and completes it, and `scheduler.h`, which parks a fiber on it.
 */

#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <cstdint>  //  std::int32_t
#endif
#endif

#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    class fiber;

    /**
     *  One asynchronous I/O request in flight. Either a fiber waits for it, or a
     *  callback runs on completion (the cross-thread wake operation).
     */
    struct io_operation {
        fiber* waiter = nullptr;
        void (*onComplete)(io_operation*, void*) = nullptr;
        void* context = nullptr;
        std::int32_t result = 0;  //  bytes transferred, or -errno
        bool completed = false;
    };
}
#endif
