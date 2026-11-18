#pragma once

/** @file Umbrella header for the asynchronous storage:
 *        `async_storage` and `make_async_storage()`, the `task` its operations return, the `io_context` driving
 *        them and the conveniences `when_all()` and `io_pool`.
 *
 *        Their contents exist only where `SQLITE_ORM_WITH_ASYNC` is defined.
 */

#include "functional/coroutines/task.h"
#include "functional/coroutines/when_all.h"
#include "async/io_context.h"
#include "async/io_pool.h"
#include "execution/async_storage.h"
