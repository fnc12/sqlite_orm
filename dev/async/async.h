#pragma once

/**
 *  Asynchronous storage: `co_await` over the ordinary storage with real
 *  asynchronous file I/O (io_uring). Available only where
 *  SQLITE_ORM_ASYNC_SUPPORTED is defined, see config.h.
 */
#include "config.h"

#ifdef SQLITE_ORM_ASYNC_SUPPORTED
#include "context_switch.h"
#include "fiber.h"
#include "io_operation.h"
#include "io_uring_engine.h"
#include "scheduler.h"
#include "async_vfs.h"
#include "task.h"
#include "io_context.h"
#include "when_all.h"
#include "io_pool.h"
#include "async_storage.h"
#endif
