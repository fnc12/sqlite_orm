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
            sqlite3_stmt* stmt = nullptr;
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
#if SQLITE_VERSION_NUMBER >= 3026000 and defined(SQLITE_ENABLE_NORMALIZE)
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

            prepared_statement_t(T t_, sqlite3_stmt* stmt_, connection_ref con_) :
                prepared_statement_base{stmt_, std::move(con_)}, t(std::move(t_)) {}
        };

        template<class T>
        struct is_prepared_statement : std::false_type {};

        template<class T>
        struct is_prepared_statement<prepared_statement_t<T>> : std::true_type {};

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

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class R, class... Args>
        struct get_all_optional_t {
            using type = T;
            using return_type = R;

            using conditions_type = std::tuple<Args...>;

            conditions_type conditions;
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

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

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Ids>
        struct get_optional_t {
            using type = T;
            using ids_type = std::tuple<Ids...>;

            ids_type ids;
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

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

        template<class T>
        struct is_insert : std::false_type {};

        template<class T>
        struct is_insert<insert_t<T>> : std::true_type {};

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

        template<class T>
        struct is_replace : std::false_type {};

        template<class T>
        struct is_replace<replace_t<T>> : std::true_type {};

        template<class It, class L, class O>
        struct insert_range_t {
            using iterator_type = It;
            using container_object_type = typename std::iterator_traits<iterator_type>::value_type;
            using transformer_type = L;
            using object_type = O;

            std::pair<iterator_type, iterator_type> range;
            transformer_type transformer;
        };

        template<class T>
        struct is_insert_range : std::false_type {};

        template<class It, class L, class O>
        struct is_insert_range<insert_range_t<It, L, O>> : std::true_type {};

        template<class It, class L, class O>
        struct replace_range_t {
            using iterator_type = It;
            using container_object_type = typename std::iterator_traits<iterator_type>::value_type;
            using transformer_type = L;
            using object_type = O;

            std::pair<iterator_type, iterator_type> range;
            transformer_type transformer;
        };

        template<class T>
        struct is_replace_range : std::false_type {};

        template<class It, class L, class O>
        struct is_replace_range<replace_range_t<It, L, O>> : std::true_type {};

        struct default_transformer {

            template<class T>
            const T& operator()(const T& object) const {
                return object;
            }
        };
    }

    /**
     *  Create a replace range statement
     *  @example
     *  ```
     *  std::vector<User> users;
     *  users.push_back(User{1, "Leony"});
     *  auto statement = storage.prepare(replace_range(users.begin(), users.end()));
     *  storage.execute(statement);
     */
    template<class It>
    internal::replace_range_t<It, internal::default_transformer, typename std::iterator_traits<It>::value_type>
    replace_range(It from, It to) {
        return {{std::move(from), std::move(to)}};
    }

    /**
     *  Create an replace range statement with explicit transformer. Transformer is used to apply containers with no strict objects with other kind of objects like pointers,
     *  optionals or whatever.
     *  @example
     *  ```
     *  std::vector<std::unique_ptr<User>> userPointers;
     *  userPointers.push_back(std::make_unique<User>(1, "Eneli"));
     *  auto statement = storage.prepare(replace_range<User>(userPointers.begin(), userPointers.end(), [](const std::unique_ptr<User> &userPointer) -> const User & {
     *      return *userPointer;
     *  }));
     *  storage.execute(statement);
     *  ```
     */
    template<class T, class It, class L>
    internal::replace_range_t<It, L, T> replace_range(It from, It to, L transformer) {
        return {{std::move(from), std::move(to)}, std::move(transformer)};
    }

    /**
     *  Create an insert range statement
     *  @example
     *  ```
     *  std::vector<User> users;
     *  users.push_back(User{1, "Leony"});
     *  auto statement = storage.prepare(insert_range(users.begin(), users.end()));
     *  storage.execute(statement);
     *  ```
     */
    template<class It>
    internal::insert_range_t<It, internal::default_transformer, typename std::iterator_traits<It>::value_type>
    insert_range(It from, It to) {
        return {{std::move(from), std::move(to)}, internal::default_transformer{}};
    }

    /**
     *  Create an insert range statement with explicit transformer. Transformer is used to apply containers with no strict objects with other kind of objects like pointers,
     *  optionals or whatever.
     *  @example
     *  ```
     *  std::vector<std::unique_ptr<User>> userPointers;
     *  userPointers.push_back(std::make_unique<User>(1, "Eneli"));
     *  auto statement = storage.prepare(insert_range<User>(userPointers.begin(), userPointers.end(), [](const std::unique_ptr<User> &userPointer) -> const User & {
     *      return *userPointer;
     *  }));
     *  storage.execute(statement);
     *  ```
     */
    template<class T, class It, class L>
    internal::insert_range_t<It, L, T> insert_range(It from, It to, L transformer) {
        return {{std::move(from), std::move(to)}, std::move(transformer)};
    }
    /**
     *  Create a replace statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.replace(myUserInstance);
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.replace(std::ref(myUserInstance));
     */
    template<class T>
    internal::replace_t<T> replace(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create an insert statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.insert(myUserInstance);
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.insert(std::ref(myUserInstance));
     */
    template<class T>
    internal::insert_t<T> insert(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create an explicit insert statement.
     *  T is an object type mapped to a storage.
     *  Cols is columns types aparameter pack. Must contain member pointers
     *  Usage: storage.insert(myUserInstance, columns(&User::id, &User::name));
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.insert(std::ref(myUserInstance), columns(&User::id, &User::name));
     */
    template<class T, class... Cols>
    internal::insert_explicit<T, Cols...> insert(T obj, internal::columns_t<Cols...> cols) {
        return {std::move(obj), std::move(cols)};
    }

    /**
     *  Create a remove statement
     *  T is an object type mapped to a storage.
     *  Usage: remove<User>(5);
     */
    template<class T, class... Ids>
    internal::remove_t<T, Ids...> remove(Ids... ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }

    /**
     *  Create an update statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.update(myUserInstance);
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.update(std::ref(myUserInstance));
     */
    template<class T>
    internal::update_t<T> update(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create a get statement.
     *  T is an object type mapped to a storage.
     *  Usage: get<User>(5);
     */
    template<class T, class... Ids>
    internal::get_t<T, Ids...> get(Ids... ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }

    /**
     *  Create a get pointer statement.
     *  T is an object type mapped to a storage.
     *  Usage: get_pointer<User>(5);
     */
    template<class T, class... Ids>
    internal::get_pointer_t<T, Ids...> get_pointer(Ids... ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    /**
     *  Create a get optional statement.
     *  T is an object type mapped to a storage.
     *  Usage: get_optional<User>(5);
     */
    template<class T, class... Ids>
    internal::get_optional_t<T, Ids...> get_optional(Ids... ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

    /**
     *  Create a remove all statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.remove_all<User>(...);
     */
    template<class T, class... Args>
    internal::remove_all_t<T, Args...> remove_all(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

    /**
     *  Create a get all statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.get_all<User>(...);
     */
    template<class T, class... Args>
    internal::get_all_t<T, std::vector<T>, Args...> get_all(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

    /**
     *  Create a get all statement.
     *  T is an object type mapped to a storage.
     *  R is a container type. std::vector<T> is default
     *  Usage: storage.get_all<User>(...);
    */
    template<class T, class R, class... Args>
    internal::get_all_t<T, R, Args...> get_all(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

    /**
     *  Create an update all statement.
     *  Usage: storage.update_all(set(...), ...);
     */
    template<class... Args, class... Wargs>
    internal::update_all_t<internal::set_t<Args...>, Wargs...> update_all(internal::set_t<Args...> set, Wargs... wh) {
        using args_tuple = std::tuple<Wargs...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Wargs>(wh)...};
        return {std::move(set), move(conditions)};
    }

    /**
     *  Create a get all pointer statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.get_all_pointer<User>(...);
     */
    template<class T, class... Args>
    internal::get_all_pointer_t<T, std::vector<std::unique_ptr<T>>, Args...> get_all_pointer(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }
    /**
     *  Create a get all pointer statement.
     *  T is an object type mapped to a storage.
     *  R is a container return type. std::vector<std::unique_ptr<T>> is default
     *  Usage: storage.get_all_pointer<User>(...);
    */
    template<class T, class R, class... Args>
    internal::get_all_pointer_t<T, R, Args...> get_all_pointer(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    /**
     *  Create a get all optional statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.get_all_optional<User>(...);
     */
    template<class T, class... Args>
    internal::get_all_optional_t<T, std::vector<std::optional<T>>, Args...> get_all_optional(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }

    /**
     *  Create a get all optional statement.
     *  T is an object type mapped to a storage.
     *  R is a container return type. std::vector<std::optional<T>> is default
     *  Usage: storage.get_all_optional<User>(...);
     */
    template<class T, class R, class... Args>
    internal::get_all_optional_t<T, R, Args...> get_all_optional(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        args_tuple conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
}
