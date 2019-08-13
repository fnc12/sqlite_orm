#pragma once

#include <functional>   //  std::function

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  Class used as a guard for a transaction. Calls `ROLLBACK` in destructor.
         *  Has explicit `commit()` and `rollback()` functions. After explicit function is fired
         *  guard won't do anything in d-tor. Also you can set `commit_on_destroy` to true to
         *  make it call `COMMIT` on destroy.
         */
        struct transaction_guard_t {
            /**
             *  This is a public lever to tell a guard what it must do in its destructor
             *  if `gotta_fire` is true
             */
            bool commit_on_destroy = false;
            
            transaction_guard_t(std::function<void()> commit_func_, std::function<void()> rollback_func_):
            commit_func(std::move(commit_func_)),
            rollback_func(std::move(rollback_func_))
            {}
            
            ~transaction_guard_t() {
                if(this->gotta_fire){
                    if(!this->commit_on_destroy){
                        this->rollback_func();
                    }else{
                        this->commit_func();
                    }
                }
            }
            
            /**
             *  Call `COMMIT` explicitly. After this call
             *  guard will not call `COMMIT` or `ROLLBACK`
             *  in its destructor.
             */
            void commit() {
                this->commit_func();
                this->gotta_fire = false;
            }
            
            /**
             *  Call `ROLLBACK` explicitly. After this call
             *  guard will not call `COMMIT` or `ROLLBACK`
             *  in its destructor.
             */
            void rollback() {
                this->rollback_func();
                this->gotta_fire = false;
            }
            
        protected:
            std::function<void()> commit_func;
            std::function<void()> rollback_func;
            bool gotta_fire = true;
        };
    }
}
