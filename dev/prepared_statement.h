#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  //  std::unique_ptr
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <type_traits>  //  std::integral_constant, std::bool_constant
#include <utility>  //  std::move, std::forward, std::exchange
#include <tuple>  //  std::tuple, std::tuple_element
#include <vector>  //  std::vector
#include <optional>  //  std::optional
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/gsl.h"
#include "functional/type_traits.h"
#include "functional/mpl.h"
#include "functional/index_sequence_util.h"
#include "tuple_helper/tuple_filter.h"
#include "connection_holder.h"
#include "ast/select.h"  // validate_select_clauses
#include "table_reference.h"
#include "mapped_type_proxy.h"
#include "vocabulary/node_traits.h"
#include "vocabulary/node_algorithms.h"  // is_bindable_v

namespace sqlite_orm::internal {
    struct prepared_statement_base {
        orm_gsl::owner<sqlite3_stmt*> stmt = nullptr;
        connection_ref con;

        ~prepared_statement_base() {
            sqlite3_finalize(this->stmt);
        }

        std::string sql() const {
            // note: sqlite3 internally checks for null before calling
            // sqlite3_normalized_sql() or sqlite3_expanded_sql(), so check here, too, even if superfluous
            if (orm_gsl::czstring sql = sqlite3_sql(this->stmt)) {
                return sql;
            } else {
                return {};
            }
        }

#if SQLITE_VERSION_NUMBER >= 3014000
        std::string expanded_sql() const {
            // note: must check return value due to SQLITE_OMIT_TRACE
#ifndef SQLITE_ORM_CLANG_MSVC
            using char_ptr = std::unique_ptr<char[], std::integral_constant<decltype(&sqlite3_free), sqlite3_free>>;
#else
            struct sqlite3_memory_deleter {
                SQLITE_ORM_STATIC_CALLOP void operator()(void* mem) SQLITE_ORM_OR_CONST_CALLOP noexcept {
                    sqlite3_free(mem);
                }
            };
            using char_ptr = std::unique_ptr<char[], sqlite3_memory_deleter>;
#endif

            if (char_ptr sql{sqlite3_expanded_sql(this->stmt)}) {
                return sql.get();
            } else {
                return {};
            }
        }
#endif
#if SQLITE_VERSION_NUMBER >= 3026000 and defined(SQLITE_ENABLE_NORMALIZE)
        std::string normalized_sql() const {
            if (orm_gsl::czstring sql = sqlite3_normalized_sql(this->stmt)) {
                return sql;
            } else {
                return {};
            }
        }
#endif

        std::string_view column_name(int index) const {
            return sqlite3_column_name(stmt, index);
        }

        /**
         *  sqlite3_stmt_readonly function: whether the statement makes no direct changes
         *  to the content of the database file.
         */
        int readonly() const {
            return sqlite3_stmt_readonly(this->stmt);
        }

        /**
         *  sqlite3_stmt_busy function: whether the statement has been stepped at least once
         *  but has not run to completion or been reset.
         */
        int busy() const {
            return sqlite3_stmt_busy(this->stmt);
        }

#if SQLITE_VERSION_NUMBER >= 3028000
        /**
         *  sqlite3_stmt_isexplain function: 1 if the statement is an EXPLAIN statement,
         *  2 if it is an EXPLAIN QUERY PLAN, 0 for an ordinary statement.
         */
        int is_explain() const {
            return sqlite3_stmt_isexplain(this->stmt);
        }
#endif
    };

    template<class T>
    struct prepared_statement_t : prepared_statement_base {
        using expression_type = T;

        expression_type expression;

        prepared_statement_t(T expression_, sqlite3_stmt* stmt_, connection_ref con_) :
            prepared_statement_base{stmt_, std::move(con_)}, expression(std::move(expression_)) {}

        prepared_statement_t(prepared_statement_t&& prepared_stmt) :
            prepared_statement_base{std::exchange(prepared_stmt.stmt, nullptr), std::move(prepared_stmt.con)},
            expression(std::move(prepared_stmt.expression)) {}
    };

    template<class T>
    inline constexpr bool is_prepared_statement_v = polyfill::is_specialization_of<T, prepared_statement_t>::value;

    template<class T>
    struct is_prepared_statement : std::bool_constant<is_prepared_statement_v<T>> {};

    /**
     *  T - type of object to obtain from a database
     */
    template<class T, class R, class... Args>
    struct get_all_t {
        using type = T;
        using return_type = R;

        using conditions_type = std::tuple<Args...>;

        conditions_type conditions;
    };

    template<class T, class R, class... Args>
    struct get_all_pointer_t {
        using type = T;
        using return_type = R;

        using conditions_type = std::tuple<Args...>;

        conditions_type conditions;
    };

    template<class T, class R, class... Args>
    struct get_all_optional_t {
        using type = T;
        using return_type = R;

        using conditions_type = std::tuple<Args...>;

        conditions_type conditions;
    };

    template<class T, class... Ids>
    struct get_t {
        using type = T;
        using ids_type = std::tuple<Ids...>;

        ids_type ids;
    };

    template<class T, class... Ids>
    struct get_pointer_t {
        using type = T;
        using ids_type = std::tuple<Ids...>;

        ids_type ids;
    };

    template<class T, class... Ids>
    struct get_optional_t {
        using type = T;
        using ids_type = std::tuple<Ids...>;

        ids_type ids;
    };

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
