#pragma once

/** @file The SET clause of an UPDATE, in both of the DSL spellings sqlite_orm offers for it -
 *        spelled out statically, or assembled at runtime.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple, std::tuple_size
#include <string>  //  std::string
#include <vector>  //  std::vector
#include <set>  //  std::set
#include <utility>  //  std::pair
#endif

#include "../../functional/type_traits.h"
#include "../../tuple_helper/tuple_traits.h"
#include "../../vocabulary/node_traits.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class... Args>
    struct set_t {
        using assigns_type = std::tuple<Args...>;

        assigns_type assigns;
    };

    template<class T>
    constexpr bool is_set_v = polyfill::is_specialization_of<T, set_t>::value;

    struct dynamic_set_entry {
        std::string serialized_value;
    };

    /**
     *  A SET clause assembled at runtime.
     *
     *  Its assignments are of different types, hence each is serialized as it is pushed back. The tables named on
     *  the left-hand side of the assignments, needed later to serialize an UPDATE, are collected at the same time,
     *  while the assignment is still at hand.
     */
    template<class C>
    struct dynamic_set_t {
        using context_t = C;
        using entry_t = dynamic_set_entry;
        using const_iterator = typename std::vector<entry_t>::const_iterator;
        using table_name_set = std::set<std::pair<std::string, std::string>>;

        dynamic_set_t(const context_t& context_) : context(context_) {}

        dynamic_set_t(const dynamic_set_t& other) = default;
        dynamic_set_t(dynamic_set_t&& other) = default;
        dynamic_set_t& operator=(const dynamic_set_t& other) = default;
        dynamic_set_t& operator=(dynamic_set_t&& other) = default;

        /**
         *  Serializes the assignment and collects the table named on its left-hand side.
         *  Defined in `implementations/dynamic_set_definitions.h`.
         */
        template<class T, satisfies<is_assign, T> = true>
        void push_back(T assign);

        const_iterator begin() const {
            return this->entries.begin();
        }

        const_iterator end() const {
            return this->entries.end();
        }

        void clear() {
            this->entries.clear();
            this->table_names.clear();
        }

        std::vector<entry_t> entries;
        context_t context;
        table_name_set table_names;
    };

    template<class T>
    constexpr bool is_dynamic_set_v = polyfill::is_specialization_of<T, dynamic_set_t>::value;

    template<class T>
    constexpr bool is_any_set_v = std::disjunction<is_set<T>, is_dynamic_set<T>>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  SET keyword used in UPDATE ... SET queries.
     *  Args must have `assign_t` type. E.g. set(assign(&User::id, 5)) or set(c(&User::id) = 5)
     */
    template<class... Args>
    internal::set_t<Args...> set(Args... args) {
        using arg_tuple = std::tuple<Args...>;
        static_assert(std::tuple_size<arg_tuple>::value == internal::count_tuple<arg_tuple, internal::is_assign>::value,
                      "set function accepts assign operators only");
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  SET keyword used in UPDATE ... SET queries. It is dynamic version. It means use can add amount of arguments now known at compilation time but known at runtime.
     */
    template<class S>
    internal::dynamic_set_t<internal::serializer_context<typename S::db_objects_type>> dynamic_set(const S& storage) {
        return {obtain_db_objects(storage)};
    }
}
