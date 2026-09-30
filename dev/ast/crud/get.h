#pragma once

/** @file The statements reading mapped objects: by primary key (get, get_pointer, get_optional),
 *        and all objects of a table satisfying the given conditions (get_all, get_all_pointer, get_all_optional).
 *
 *        The three statements of either group are DSL spellings of one and the same SELECT; they differ only in
 *        how an object is handed out - as is, owned by a `std::unique_ptr`, or held by a `std::optional`. Each of
 *        them declares that as its `result_type`: the type of the object read by primary key, or of the elements
 *        of the container the objects are read into.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::disjunction
#include <tuple>  //  std::tuple, std::tuple_element
#include <memory>  //  std::unique_ptr
#include <optional>  //  std::optional
#include <vector>  //  std::vector
#include <utility>  //  std::forward
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/type_traits.h"
#include "../../functional/mpl.h"
#include "../../functional/index_sequence_util.h"
#include "../../tuple_helper/tuple_filter.h"
#include "../../table_reference.h"
#include "../../mapped_type_proxy.h"
#include "../../vocabulary/node_traits.h"
#include "../../vocabulary/node_algorithms.h"  //  is_bindable_v
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../select.h"  //  validate_select_clauses

namespace sqlite_orm::internal {
    template<class T, class... Ids>
    struct get_t {
        using type = T;
        using result_type = T;
        using ids_type = std::tuple<Ids...>;

        ids_type ids;
    };

    template<class T, class... Ids>
    struct get_pointer_t {
        using type = T;
        using result_type = std::unique_ptr<T>;
        using ids_type = std::tuple<Ids...>;

        ids_type ids;
    };

    template<class T, class... Ids>
    struct get_optional_t {
        using type = T;
        using result_type = std::optional<T>;
        using ids_type = std::tuple<Ids...>;

        ids_type ids;
    };

    template<class T>
    constexpr bool is_any_get_by_id_v = std::disjunction<polyfill::is_specialization_of<T, get_t>,
                                                         polyfill::is_specialization_of<T, get_pointer_t>,
                                                         polyfill::is_specialization_of<T, get_optional_t>>::value;

    /**
     *  T - type of object to obtain from a database
     *  R - container type the objects are read into
     */
    template<class T, class R, class... Args>
    struct get_all_t {
        using type = T;
        using result_type = mapped_type_proxy_t<T>;
        using return_type = R;

        using conditions_type = std::tuple<Args...>;

        conditions_type conditions;
    };

    template<class T, class R, class... Args>
    struct get_all_pointer_t {
        using type = T;
        using result_type = std::unique_ptr<T>;
        using return_type = R;

        using conditions_type = std::tuple<Args...>;

        conditions_type conditions;
    };

    template<class T, class R, class... Args>
    struct get_all_optional_t {
        using type = T;
        using result_type = std::optional<T>;
        using return_type = R;

        using conditions_type = std::tuple<Args...>;

        conditions_type conditions;
    };

    template<class T>
    constexpr bool is_any_get_all_v = std::disjunction<polyfill::is_specialization_of<T, get_all_t>,
                                                       polyfill::is_specialization_of<T, get_all_pointer_t>,
                                                       polyfill::is_specialization_of<T, get_all_optional_t>>::value;

    template<class T, class Tpl>
    constexpr void validate_get_all_conditions() {
        using from2_index_sequence = filter_tuple_sequence_t<Tpl, is_from2>;
        if constexpr (from2_index_sequence::size() > 0) {
            using from_type = std::tuple_element_t<index_sequence_value_at<0>(from2_index_sequence{}), Tpl>;
            // check whether one of table expressions' type is the same as the requested table type
            static_assert(mpl::invoke_t<mpl::contains<check_if_projected_is_type<type_t, T>>, from_type>::value,
                          "Requested object type must be listed in explicit FROM clause");
        }
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Create a get statement.
     *  T is an object type mapped to a storage.
     *  Usage: get<User>(5);
     */
    template<class T, class... Ids>
    internal::get_t<T, Ids...> get(Ids... ids) {
        static_assert((internal::is_bindable_v<internal::value_unref_type_t<Ids>> && ...),
                      "Only primary key values are accepted as Ids");
        return {{std::forward<Ids>(ids)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: get<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto get(Ids... ids) {
        return get<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

    /**
     *  Create a get pointer statement.
     *  T is an object type mapped to a storage.
     *  Usage: get_pointer<User>(5);
     */
    template<class T, class... Ids>
    internal::get_pointer_t<T, Ids...> get_pointer(Ids... ids) {
        static_assert((internal::is_bindable_v<internal::value_unref_type_t<Ids>> && ...),
                      "Only primary key values are accepted as Ids");
        return {{std::forward<Ids>(ids)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get pointer statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: get_pointer<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto get_pointer(Ids... ids) {
        return get_pointer<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

    /**
     *  Create a get optional statement.
     *  T is an object type mapped to a storage.
     *  Usage: get_optional<User>(5);
     */
    template<class T, class... Ids>
    internal::get_optional_t<T, Ids...> get_optional(Ids... ids) {
        static_assert((internal::is_bindable_v<internal::value_unref_type_t<Ids>> && ...),
                      "Only primary key values are accepted as Ids");
        return {{std::forward<Ids>(ids)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get optional statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: get_optional<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto get_optional(Ids... ids) {
        return get_optional<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

    /**
     *  Create a get all statement.
     *  T is an explicitly specified object mapped to a storage or a table alias.
     *  R is a container type. std::vector<T> is default
     *  Usage: storage.prepare(get_all<User>(...));
     */
    template<class T, class R = std::vector<internal::mapped_type_proxy_t<T>>, class... Args>
    internal::get_all_t<T, R, Args...> get_all(Args... conditions) {
        using conditions_tuple = std::tuple<Args...>;
        internal::validate_select_clauses<conditions_tuple>();
        internal::validate_get_all_conditions<T, conditions_tuple>();
        return {{std::forward<Args>(conditions)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get all statement.
     *  `mapped` is an explicitly specified table reference or table alias to be extracted.
     *  `R` is the container return type, which must have a `R::push_back(T&&)` method, and defaults to `std::vector<T>`
     *  Usage: storage.get_all<sqlite_schema>(...);
     */
    template<orm_refers_to_table auto mapped,
             class R = std::vector<internal::mapped_type_proxy_t<decltype(mapped)>>,
             class... Args>
    auto get_all(Args&&... conditions) {
        return get_all<internal::auto_decay_table_ref_t<mapped>, R>(std::forward<Args>(conditions)...);
    }
#endif

    /**
     *  Create a get all pointer statement.
     *  T is an object type mapped to a storage.
     *  R is a container return type. std::vector<std::unique_ptr<T>> is default
     *  Usage: storage.prepare(get_all_pointer<User>(...));
     */
    template<class T, class R = std::vector<std::unique_ptr<T>>, class... Args>
    internal::get_all_pointer_t<T, R, Args...> get_all_pointer(Args... conditions) {
        using conditions_tuple = std::tuple<Args...>;
        internal::validate_select_clauses<conditions_tuple>();
        internal::validate_get_all_conditions<T, conditions_tuple>();
        return {{std::forward<Args>(conditions)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get all pointer statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  R is a container return type. std::vector<std::unique_ptr<T>> is default
     *  Usage: storage.prepare(get_all_pointer<user_table>(...));
     */
    template<orm_table_reference auto table,
             class R = std::vector<internal::auto_decay_table_ref_t<table>>,
             class... Args>
    auto get_all_pointer(Args... conditions) {
        return get_all_pointer<internal::auto_decay_table_ref_t<table>, R>(std::forward<Args>(conditions)...);
    }
#endif

    /**
     *  Create a get all optional statement.
     *  T is an object type mapped to a storage.
     *  R is a container return type. std::vector<std::optional<T>> is default
     *  Usage: storage.get_all_optional<User>(...);
     */
    template<class T, class R = std::vector<std::optional<T>>, class... Args>
    internal::get_all_optional_t<T, R, Args...> get_all_optional(Args... conditions) {
        using conditions_tuple = std::tuple<Args...>;
        internal::validate_select_clauses<conditions_tuple>();
        internal::validate_get_all_conditions<T, conditions_tuple>();
        return {{std::forward<Args>(conditions)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get all optional statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  R is a container return type. std::vector<std::optional<T>> is default
     *  Usage: storage.get_all_optional<user_table>(...);
     */
    template<orm_table_reference auto table,
             class R = std::vector<internal::auto_decay_table_ref_t<table>>,
             class... Args>
    auto get_all_optional(Args&&... conditions) {
        return get_all_optional<internal::auto_decay_table_ref_t<table>, R>(std::forward<Args>(conditions)...);
    }
#endif
}
