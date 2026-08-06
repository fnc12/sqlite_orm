#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {
    struct basic_generated_always {
        enum class storage_type {
            not_specified,
            virtual_,
            stored,
        };

#if SQLITE_VERSION_NUMBER >= 3031000
        bool _full = true;
        storage_type _storage = storage_type::not_specified;
#endif
    };

#if SQLITE_VERSION_NUMBER >= 3031000
    template<class T>
    struct generated_always_t : basic_generated_always {
        using expression_type = T;

        expression_type _expression;

        constexpr generated_always_t<T> virtual_() && {
            return {_full, storage_type::virtual_, std::move(_expression)};
        }

        constexpr generated_always_t<T> stored() && {
            return {_full, storage_type::stored, std::move(_expression)};
        }
    };
#endif

    template<class T>
    constexpr bool is_generated_always_v =
#if SQLITE_VERSION_NUMBER >= 3031000
        polyfill::is_specialization_of<T, generated_always_t>::value;
#else
        false;
#endif

    template<class T>
    struct is_generated_always : polyfill::bool_constant<is_generated_always_v<T>> {};
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3031000
    template<class T>
    constexpr internal::generated_always_t<T> generated_always_as(T expression) {
        return {true, internal::basic_generated_always::storage_type::not_specified, std::move(expression)};
    }

    template<class T>
    constexpr internal::generated_always_t<T> as(T expression) {
        return {false, internal::basic_generated_always::storage_type::not_specified, std::move(expression)};
    }
#endif
}
