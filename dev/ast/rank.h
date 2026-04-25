#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::forward
#endif

#include "window.h"

namespace sqlite_orm::internal {
    struct rank_t {
        template<class... OverArgs>
        over_t<rank_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  RANK() window function
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::rank_t rank() {
        return {};
    }
}
