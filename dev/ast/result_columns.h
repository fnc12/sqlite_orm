#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple, std::tuple_size
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tuple_helper/tuple_traits.h"
#include "../alias_traits.h"
#include "../vocabulary/node_algorithms.h"  // is_select_clause
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/structural_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct as_optional_t {
        using expression_type = T;

        expression_type expression;
    };

    template<class T>
    constexpr bool is_as_optional_v = polyfill::is_specialization_of<T, as_optional_t>::value;

    template<class... Args>
    struct columns_t {
        using columns_type = std::tuple<Args...>;

        columns_type columns;

        static constexpr int count = std::tuple_size<columns_type>::value;
    };

    template<class T>
    constexpr bool is_columns_v = polyfill::is_specialization_of<T, columns_t>::value;

    /*
     *  Captures the type of an aggregate/structure/object and column expressions, such that
     *  `T` can be constructed in-place as part of a result row.
     *  `T` must be constructible using direct-list-initialization.
     */
    template<class T, class... Args>
    struct struct_t {
        using object_type = T;
        using columns_type = std::tuple<Args...>;

        columns_type columns;

        static constexpr int count = std::tuple_size<columns_type>::value;
    };

    template<class T>
    constexpr bool is_struct_v = polyfill::is_specialization_of<T, struct_t>::value;

    template<class T>
    struct asterisk_t {
        using type = T;

        bool defined_order = false;
    };

    template<class T>
    constexpr bool is_asterisk_v = polyfill::is_specialization_of<T, asterisk_t>::value;

    template<class T>
    struct object_t {
        using type = T;

        bool defined_order = false;
    };

    template<class T>
    constexpr bool is_object_node_v = polyfill::is_specialization_of<T, object_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    internal::as_optional_t<T> as_optional(T value) {
        return {std::move(value)};
    }

    /*
     *  Combine multiple columns in a tuple.
     */
    template<class... Args>
    constexpr internal::columns_t<Args...> columns(Args... args) {
        static_assert(internal::count_tuple<std::tuple<Args...>, internal::is_select_clause>::value == 0,
                      "a statement clause cannot be used as a column expression");
        return {{std::forward<Args>(args)...}};
    }

    /*
     *  Construct an unmapped structure ad-hoc from multiple columns.
     *  `T` must be constructible from the column results using direct-list-initialization.
     */
    template<class T, class... Args>
    constexpr internal::struct_t<T, Args...> struct_(Args... args) {
        static_assert(internal::count_tuple<std::tuple<Args...>, internal::is_select_clause>::value == 0,
                      "a statement clause cannot be used as a column expression");
        return {{std::forward<Args>(args)...}};
    }

    /**
     *  `SELECT * FROM T` expression that fetches results as tuples.
     *  T is a type mapped to a storage, or an alias of it.
     *  The `definedOrder` parameter denotes the expected order of result columns.
     *  The default is the implicit order as returned by SQLite, which may differ from the defined order
     *  if the schema of a table has been changed.
     *  By specifying the defined order, the columns are written out in the resulting select SQL string.
     *  
     *  In pseudo code:
     *  select(asterisk<User>(false)) -> SELECT * from User
     *  select(asterisk<User>(true))  -> SELECT id, name from User
     *  
     *  Example: auto rows = storage.select(asterisk<User>());
     *  // decltype(rows) is std::vector<std::tuple<...all columns in implicitly stored order...>>
     *  Example: auto rows = storage.select(asterisk<User>(true));
     *  // decltype(rows) is std::vector<std::tuple<...all columns in declared make_table order...>>
     *  
     *  If you need to fetch results as objects instead of tuples please use `object<T>()`.
     */
    template<class T>
    constexpr internal::asterisk_t<T> asterisk(bool definedOrder = false) {
        return {definedOrder};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Example:
     *  constexpr orm_table_alias auto m = "m"_alias.for_<Employee>();
     *  auto reportingTo = 
     *  storage.select(asterisk<m>(), inner_join<m>(on(m->*&Employee::reportsTo == &Employee::employeeId)));
     */
    template<orm_refers_to_recordset auto recordset>
    constexpr auto asterisk(bool definedOrder = false) {
        return asterisk<internal::auto_decay_table_ref_t<recordset>>(definedOrder);
    }
#endif

    /**
     *  `SELECT * FROM T` expression that fetches results as objects of type T.
     *  T is a type mapped to a storage, or an alias of it.
     *  
     *  Example: auto rows = storage.select(object<User>());
     *  // decltype(rows) is std::vector<User>, where the User objects are constructed from columns in implicitly stored order
     *  Example: auto rows = storage.select(object<User>(true));
     *  // decltype(rows) is std::vector<User>, where the User objects are constructed from columns in declared make_table order
     *  
     *  If you need to fetch results as tuples instead of objects please use `asterisk<T>()`.
     */
    template<class T>
    constexpr internal::object_t<T> object(bool definedOrder = false) {
        return {definedOrder};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_table auto als>
    constexpr auto object(bool definedOrder = false) {
        return object<internal::auto_decay_table_ref_t<als>>(definedOrder);
    }
#endif
}
