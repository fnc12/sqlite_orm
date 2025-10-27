#pragma once

namespace sqlite_orm::internal {
    /*  
        Poor-man's scope (exit) guard until C++29 finally comes with proper standard facilities [Draft D3610].
     */
    template<class F>
    struct scope_guard {
        explicit scope_guard(F f) : f{std::move(f)} {}
        ~scope_guard() {
            f();
        }

        F f;
    };
}
