#pragma once

#include <sqlite3.h>
#include <iterator>  //  std::iterator_traits
#include <string>  //  std::string
#include <type_traits>  //  std::true_type, std::false_type
#include <utility>  //  std::pair

#include "connection_holder.h"
#include "select_constraints.h"

namespace sqlite_orm {

    namespace internal {

        struct prepared_statement_base {
            sqlite3_stmt *stmt = nullptr;
            connection_ref con;

            ~prepared_statement_base() {
                if(this->stmt) {
                    sqlite3_finalize(this->stmt);
                    this->stmt = nullptr;
                }
            }

            std::string sql() const {
                if(this->stmt) {
                    if(auto res = sqlite3_sql(this->stmt)) {
                        return res;
                    } else {
                        return {};
                    }
                } else {
                    return {};
                }
            }

#if SQLITE_VERSION_NUMBER >= 3014000
            std::string expanded_sql() const {
                if(this->stmt) {
                    if(auto res = sqlite3_expanded_sql(this->stmt)) {
                        std::string result = res;
                        sqlite3_free(res);
                        return result;
                    } else {
                        return {};
                    }
                } else {
                    return {};
                }
            }
#endif
#if SQLITE_VERSION_NUMBER >= 3027000
            std::string normalized_sql() const {
                if(this->stmt) {
                    if(auto res = sqlite3_normalized_sql(this->stmt)) {
                        return res;
                    } else {
                        return {};
                    }
                } else {
                    return {};
                }
            }
#endif
        };

        template<class T>
        struct prepared_statement_t : prepared_statement_base {
            using expression_type = T;

            expression_type t;

            prepared_statement_t(T t_, sqlite3_stmt *stmt, connection_ref con_) :
                prepared_statement_base{stmt, std::move(con_)}, t(std::move(t_)) {}
        };

        template<class T, class... Args>
        struct get_all_t {
            using type = T;

            using conditions_type = std::tuple<Args...>;

            conditions_type conditions;
        };

        template<class T, class... Args>
        struct get_all_pointer_t {
            using type = T;

            using conditions_type = std::tuple<Args...>;

            conditions_type conditions;
        };

        template<class T, class... Wargs>
        struct update_all_t;

        template<class... Args, class... Wargs>
        struct update_all_t<set_t<Args...>, Wargs...> {
            using set_type = set_t<Args...>;
            using conditions_type = std::tuple<Wargs...>;

            set_type set;
            conditions_type conditions;
        };

        template<class T, class... Args>
        struct remove_all_t {
            using type = T;

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

        template<class T>
        struct update_t {
            using type = T;

            type obj;
        };

        template<class T, class... Ids>
        struct remove_t {
            using type = T;

            using ids_type = std::tuple<Ids...>;

            ids_type ids;
        };

        template<class T>
        struct insert_t {
            using type = T;

            type obj;
        };

        template<class T, class... Cols>
        struct insert_explicit {
            using type = T;
            using columns_type = columns_t<Cols...>;

            type obj;
            columns_type columns;
        };

        template<class T>
        struct replace_t {
            using type = T;

            type obj;
        };

        template<class It>
        struct insert_range_t {
            using iterator_type = It;
            using object_type = typename std::iterator_traits<iterator_type>::value_type;

            std::pair<iterator_type, iterator_type> range;
        };

        template<class It>
        struct replace_range_t {
            using iterator_type = It;
            using object_type = typename std::iterator_traits<iterator_type>::value_type;

            std::pair<iterator_type, iterator_type> range;
        };
    }

    /**
     *  Create a replace range statement
     */
    template<class It>
    internal::replace_range_t<It> replace_range(It from, It to) {
        return {{std::move(from), std::move(to)}};
    }

    /**
     *  Create an insert range statement
     */
    template<class It>
    internal::insert_range_t<It> insert_range(It from, It to) {
        return {{std::move(from), std::move(to)}};
    }

    /**
     *  Create a replace by reference statement
     *  Usage: replace(myUserInstance);
     */
    template<class T>
    internal::replace_t<T> replace(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create an insert by reference statement
     *  Usage: insert(myUserInstance);
     */
    template<class T>
    internal::insert_t<T> insert(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create an explicit insert by reference statement.
     *  Usage: insert(myUserInstance, columns(&User::id, &User::name));
     */
    template<class T, class... Cols>
    internal::insert_explicit<T, Cols...> insert(T obj, internal::columns_t<Cols...> cols) {
        return {std::move(obj), std::move(cols)};
    }

    /**
     *  Create a remove statement
     *  Usage: remove<User>(5);
     */
    template<class T, class... Ids>
    internal::remove_t<T, Ids...> remove(Ids... ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }

    /**
     *  Create an update by reference statement.
     *  Usage: update(myUserInstance);
     */
    template<class T>
    internal::update_t<T> update(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create a get statement.
     *  Usage: get<User>(5);
     */
    template<class T, class... Ids>
    internal::get_t<T, Ids...> get(Ids... ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }

    /**
     *  Create a get pointer statement.
     *  Usage: get_pointer<User>(5);
     */
    template<class T, class... Ids>
    internal::get_pointer_t<T, Ids...> get_pointer(Ids... ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }

    /**
     *  Create a remove all statement.
     *  Usage: remove_all<User>(...);
     */
    template<class T, class... Args>
    internal::remove_all_t<T, Args...> remove_all(Args... args) {
        std::tuple<Args...> conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

    /**
     *  Create a get all statement.
     *  Usage: get_all<User>(...);
     */
    template<class T, class... Args>
    internal::get_all_t<T, Args...> get_all(Args... args) {
        std::tuple<Args...> conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

    /**
     *  Create an update all statement.
     *  Usage: update_all(set(...), ...);
     */
    template<class... Args, class... Wargs>
    internal::update_all_t<internal::set_t<Args...>, Wargs...> update_all(internal::set_t<Args...> set, Wargs... wh) {
        std::tuple<Wargs...> conditions{std::forward<Wargs>(wh)...};
        return {std::move(set), move(conditions)};
    }

    template<class T, class... Args>
    internal::get_all_pointer_t<T, Args...> get_all_pointer(Args... args) {
        std::tuple<Args...> conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

    template<int N, class It>
    auto &get(internal::prepared_statement_t<internal::insert_range_t<It>> &statement) {
        static_assert(N == 0 || N == 1, "get<> works only with [0; 1] argument for insert range statement");
        return std::get<N>(statement.t.range);
    }

    template<int N, class It>
    const auto &get(const internal::prepared_statement_t<internal::insert_range_t<It>> &statement) {
        static_assert(N == 0 || N == 1, "get<> works only with [0; 1] argument for insert range statement");
        return std::get<N>(statement.t.range);
    }

    template<int N, class It>
    auto &get(internal::prepared_statement_t<internal::replace_range_t<It>> &statement) {
        static_assert(N == 0 || N == 1, "get<> works only with [0; 1] argument for replace range statement");
        return std::get<N>(statement.t.range);
    }

    template<int N, class It>
    const auto &get(const internal::prepared_statement_t<internal::replace_range_t<It>> &statement) {
        static_assert(N == 0 || N == 1, "get<> works only with [0; 1] argument for replace range statement");
        return std::get<N>(statement.t.range);
    }

    template<int N, class T, class... Ids>
    auto &get(internal::prepared_statement_t<internal::get_t<T, Ids...>> &statement) {
        return std::get<N>(statement.t.ids);
    }

    template<int N, class T, class... Ids>
    const auto &get(const internal::prepared_statement_t<internal::get_t<T, Ids...>> &statement) {
        return std::get<N>(statement.t.ids);
    }

    template<int N, class T, class... Ids>
    auto &get(internal::prepared_statement_t<internal::get_pointer_t<T, Ids...>> &statement) {
        return std::get<N>(statement.t.ids);
    }

    template<int N, class T, class... Ids>
    const auto &get(const internal::prepared_statement_t<internal::get_pointer_t<T, Ids...>> &statement) {
        return std::get<N>(statement.t.ids);
    }
}
