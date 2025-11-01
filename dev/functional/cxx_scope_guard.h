#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  // std::forward
#endif

namespace sqlite_orm::internal {
    /*  
        Poor-man's scope (exit) guard until C++29 finally comes with proper standard facilities [Draft D3610].
     */
    template<class F>
    class scope_guard {
      public:
        explicit scope_guard(F&& exitFunction) : _exitFunction{std::forward<F>(exitFunction)} {}

        ~scope_guard() {
            _exitFunction();
        }

      private:
        F _exitFunction;
    };
}
