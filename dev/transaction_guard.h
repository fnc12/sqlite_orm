#pragma once

#include <functional>  //  std::function
#include <utility>  //  std::move

#include "connection_holder.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  Class used as a guard for a transaction. Calls `ROLLBACK` in destructor.
         *  Has explicit `commit()` and `rollback()` functions. After explicit function is fired
         *  guard won't do anything in d-tor. Also you can set `commit_on_destroy` to true to
         *  make it call `COMMIT` on destroy.
         * 
         *  Note: The guard's destructor is explicitly marked as potentially throwing,
         *  so exceptions that occur during commit or rollback are propagated to the caller.
         */
        struct transaction_guard_t {
            /**
             *  This is a public lever to tell a guard what it must do in its destructor
             *  if `gotta_fire` is true
             */
            bool commit_on_destroy = false;

            transaction_guard_t(connection_ref connection_,
                                std::function<void()> commit_func_,
                                std::function<void()> rollback_func_) :
                connection(std::move(connection_)),
                commit_func(std::move(commit_func_)), rollback_func(std::move(rollback_func_)) {}

            transaction_guard_t(transaction_guard_t&& other) :
                commit_on_destroy(other.commit_on_destroy), connection(std::move(other.connection)),
                commit_func(std::move(other.commit_func)), rollback_func(std::move(other.rollback_func)),
                gotta_fire(other.gotta_fire) {
                other.gotta_fire = false;
            }

            ~transaction_guard_t() noexcept(false) {
                if(this->gotta_fire) {
                    if(this->commit_on_destroy) {
                        this->commit_func();
                    } else {
                        this->rollback_func();
                    }
                }
            }

            transaction_guard_t& operator=(transaction_guard_t&&) = delete;

            /**
             *  Call `COMMIT` explicitly. After this call
             *  guard will not call `COMMIT` or `ROLLBACK`
             *  in its destructor.
             */
            void commit() {
                this->gotta_fire = false;
                this->commit_func();
            }

            /**
             *  Call `ROLLBACK` explicitly. After this call
             *  guard will not call `COMMIT` or `ROLLBACK`
             *  in its destructor.
             */
            void rollback() {
                this->gotta_fire = false;
                this->rollback_func();
            }

          protected:
            connection_ref connection;
            std::function<void()> commit_func;
            std::function<void()> rollback_func;
            bool gotta_fire = true;
        };
    }
}
