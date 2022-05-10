#pragma once

namespace sqlite_orm {

    namespace internal {

        template<class L, class R>
        bool compare_any(const L& /*lhs*/, const R& /*rhs*/) {
            return false;
        }
        template<class O>
        bool compare_any(const O& lhs, const O& rhs) {
            return lhs == rhs;
        }
    }
}
