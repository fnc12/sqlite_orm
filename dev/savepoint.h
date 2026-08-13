#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <functional>  //  std::function
#include <utility>  //  std::move
#endif

#include "connection_holder.h"

namespace sqlite_orm::internal {
    /**
     *  Class used as a guard for a savepoint. Calls `ROLLBACK TO` and `RELEASE` in destructor.
     *  Has explicit `release()` and `rollback_to()` functions. After the `release()` function is fired
     *  the guard won't do anything in its d-tor. Note that unlike a rollback of a transaction
     *  `rollback_to()` doesn't finish the savepoint: the savepoint remains on the transaction stack,
     *  so `rollback_to()` may be called multiple times, and the guard is still armed afterwards.
     *  Also you can set `release_on_destroy` to true to make the guard call `RELEASE` only on destroy,
     *  keeping the changes.
     *
     *  Note: The guard's destructor is explicitly marked as potentially throwing,
     *  so exceptions that occur during release or rollback are propagated to the caller.
     */
    struct savepoint_t {
        /**
         *  This is a public lever to tell a guard what it must do in its destructor
         *  if `gotta_fire` is true
         */
        bool release_on_destroy = false;

        savepoint_t(connection_ref connection_,
                    std::function<void()> release_func_,
                    std::function<void()> rollback_to_func_) :
            connection(std::move(connection_)), release_func(std::move(release_func_)),
            rollback_to_func(std::move(rollback_to_func_)) {}

        savepoint_t(savepoint_t&& other) :
            release_on_destroy(other.release_on_destroy), connection(std::move(other.connection)),
            release_func(std::move(other.release_func)), rollback_to_func(std::move(other.rollback_to_func)),
            gotta_fire(other.gotta_fire) {
            other.gotta_fire = false;
        }

        ~savepoint_t() noexcept(false) {
            if (this->gotta_fire) {
                if (!this->release_on_destroy) {
                    this->rollback_to_func();
                }
                this->release_func();
            }
        }

        savepoint_t& operator=(savepoint_t&&) = delete;

        /**
         *  Call `RELEASE` explicitly. After this call
         *  guard will not call `ROLLBACK TO` or `RELEASE`
         *  in its destructor.
         */
        void release() {
            this->gotta_fire = false;
            this->release_func();
        }

        /**
         *  Call `ROLLBACK TO` explicitly. The savepoint remains on the
         *  transaction stack, so the guard is still armed after this call:
         *  its destructor will fire unless `release()` is called.
         */
        void rollback_to() {
            this->rollback_to_func();
        }

      private:
        connection_ref connection;
        std::function<void()> release_func;
        std::function<void()> rollback_to_func;
        bool gotta_fire = true;
    };
}
