#pragma once

/** @file Umbrella header for the asynchronous storage, the headers in `async/`:
 *        `async_storage` and `make_async_storage()`, the `task` its operations return, the `io_context` driving
 *        them and the conveniences `when_all()` and `io_pool`.
 *
 *        Their contents exist only where `SQLITE_ORM_WITH_ASYNC` is defined.
 */

#include "async/async_storage.h"
#include "async/io_context.h"
#include "async/io_pool.h"
#include "async/task.h"
#include "async/when_all.h"
