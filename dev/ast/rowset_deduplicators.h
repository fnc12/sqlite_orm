#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct distinct_string {
        operator std::string() const {
            return "DISTINCT";
        }
    };

    /**
     *  DISTINCT generic container.
     */
    template<class T>
    struct distinct_t : distinct_string {
        using expression_type = T;

        expression_type expression;

        distinct_t(expression_type expression) : expression(std::move(expression)) {}
    };

    struct all_string {
        operator std::string() const {
            return "ALL";
        }
    };

    /**
     *  ALL generic container.
     */
    template<class T>
    struct all_t : all_string {
        using expression_type = T;

        expression_type expression;

        all_t(expression_type expression) : expression(std::move(expression)) {}
    };

    /**
     *  Whether a type represents the DISTINCT keyword.
     */
    template<class T>
    constexpr bool is_distinct_v = polyfill::is_specialization_of<T, distinct_t>::value;

    /**
     *  Whether a type represents a keyword for a result set modifier (as part of a simple select expression).
     */
    template<class T>
    constexpr bool is_rowset_deduplicator_v =
        polyfill::disjunction<is_distinct<T>, polyfill::is_specialization_of<T, all_t>>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    internal::distinct_t<T> distinct(T t) {
        return {std::move(t)};
    }

    template<class T>
    internal::all_t<T> all(T t) {
        return {std::move(t)};
    }
}
