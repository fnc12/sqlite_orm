#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::remove_const, std::type_identity
#include <utility>  // std::move
#include <tuple>
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "alias_traits.h"
#include "literal.h"

namespace sqlite_orm::internal {
    /*
     *  Bound input arguments.
     */
    template<class Table, class... Args>
    struct table_valued_expression {
        using type = Table;
        using constraints_type = std::tuple<Args...>;

        constraints_type table_values;
    };

    template<class T>
    inline constexpr bool is_table_valued_expression_v = polyfill::is_specialization_of_v<T, table_valued_expression>;

    template<class T>
    using is_table_valued_expression = polyfill::bool_constant<is_table_valued_expression_v<T>>;

    /*
     *  Identity wrapper around a mapped object, facilitating uniform column pointer expressions and virtual tables usable as table-valued functions.
     */
    template<class O>
    struct table_reference : polyfill::type_identity<O> {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
        /** 
         *  Make a table-valued function call.
         */
        template<class... Args>
        constexpr table_valued_expression<O, Args...> operator()(Args... arguments) const {
            return {{ {std::move(arguments)}... }};
        }
#else
        /** 
         *  Make a table-valued function call.
         */
        template<class... Args>
        constexpr table_valued_expression<O, Args...> operator()(Args... arguments) const {
            return {{{std::move(arguments)}...}};
        }
#endif
    };

    template<class RecordSet>
    struct decay_table_ref : std::remove_const<RecordSet> {};
    template<class O>
    struct decay_table_ref<table_reference<O>> : polyfill::type_identity<O> {};
    template<class O>
    struct decay_table_ref<const table_reference<O>> : polyfill::type_identity<O> {};

    template<class RecordSet>
    using decay_table_ref_t = typename decay_table_ref<RecordSet>::type;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<auto recordset>
    using auto_decay_table_ref_t = typename decay_table_ref<decltype(recordset)>::type;
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class T>
    concept orm_table_valued_expression = internal::is_table_valued_expression_v<T>;
#endif

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /**
     *  Make a table reference.
     */
    template<class O>
        requires (!orm_recordset_alias<O>)
    consteval internal::table_reference<O> c() {
        return {};
    }
#else
    /**
     *  Make a table reference.
     */
    template<class O, std::enable_if_t<!internal::is_recordset_alias_v<O>, bool> = true>
    constexpr internal::table_reference<O> c() {
        return {};
    }
#endif
}
