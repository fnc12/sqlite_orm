#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <cstdint>  //  std::int32_t
#endif

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
