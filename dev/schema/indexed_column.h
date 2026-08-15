#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/node_traits.h"
#include "../vocabulary/traits/structural_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class C>
    struct indexed_column_t {
        using column_type = C;

        column_type _column_or_expression;
        std::string _collation_name;
        int _order = 0;  //  -1 = desc, 1 = asc, 0 = unspecified

        indexed_column_t<column_type> collate(std::string name) && {
            _collation_name = std::move(name);
            return std::move(*this);
        }

        indexed_column_t<column_type> asc() && {
            _order = 1;
            return std::move(*this);
        }

        indexed_column_t<column_type> desc() && {
            _order = -1;
            return std::move(*this);
        }
    };

    template<class T>
    constexpr bool is_indexed_column_v = polyfill::is_specialization_of<T, indexed_column_t>::value;

    template<class C>
    auto make_indexed_column(C col) {
        if constexpr (is_indexed_column_v<C> || is_where_v<C>) {
            return std::move(col);
        } else {
            return indexed_column_t<C>{std::move(col)};
        }
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Use this function to specify indexed column inside `make_index` function call.
     *  Example: make_index("index_name", indexed_column(&User::id).asc())
     */
    template<class C>
    internal::indexed_column_t<C> indexed_column(C column_or_expression) {
        return {std::move(column_or_expression)};
    }
}
