#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <tuple>

#include "tuple_helper/tuple_helper.h"

// NOTE Idea : Maybe also implement a custom trigger system to call a c++ callback when a trigger triggers ?
// (Could be implemented with a normal trigger that insert or update an internal table and then retreive
// the event in the C++ code, to call the C++ user callback, with update hooks: https://www.sqlite.org/c3ref/update_hook.html)
// It could be an interesting feature to bring to sqlite_orm that other libraries don't have ?

namespace sqlite_orm {
    namespace trigger {
        enum trigger_timing_t { trigger_before, trigger_after, trigger_instead_of };

        inline trigger_timing_t before() {
            return trigger_before;
        }

        inline trigger_timing_t after() {
            return trigger_after;
        }

        inline trigger_timing_t instead_of() {
            return trigger_instead_of;
        }
    }

    namespace internal {
        template<class T>
        struct on_delete_t {
            using object_type = typename T::object_type;
            using elements_type = typename T::elements_type;
            using table_type = T;

            T table;

            on_delete_t(T table) : table(table) {}

            template<class C>
            std::string to_string(const C &context) const {
                return "DELETE ON '" + context.impl.find_table_name(typeid(typename T::object_type)) + "' ";
            }
        };

        template<class T>
        struct on_insert_t {
            using object_type = typename T::object_type;
            using elements_type = typename T::elements_type;
            using table_type = T;

            T table;

            on_insert_t(T table) : table(table) {}

            template<class C>
            std::string to_string(const C &context) const {
                return "INSERT ON '" + context.impl.find_table_name(typeid(typename T::object_type)) + "' ";
            }
        };

        template<class T, class... Cs>
        struct on_update_t {
            using columns_type = std::tuple<Cs...>;
            using object_type = typename T::object_type;
            using elements_type = typename T::elements_type;
            using table_type = T;

            columns_type columns;
            T table;

            on_update_t(T table, Cs... args) :
                columns(std::make_tuple<Cs...>(std::forward<Cs>(args)...)), table(table) {}

            template<class C>
            std::string to_string(const C &context) const {
                std::stringstream ss;
                std::string sep = " OF ";

                ss << "UPDATE";

                iterate_tuple(columns, [this, &ss, &sep](auto &node) {
                    std::string name = "";
                    table.for_each_column([&node, &name](auto &c) {
                        using col_t = typename std::decay<decltype(c)>::type;
                        using member_t = typename std::decay<typename col_t::member_pointer_t>::type;
                        using node_t = typename std::decay<decltype(node)>::type;

                        if constexpr(std::is_same<member_t, node_t>::value) {
                            if(c.member_pointer == node) {
                                name = c.name;
                            }
                        }
                    });
                    ss << sep << "'" << name << "'";
                    sep = ", ";
                });
                ss << " ON '" << context.impl.find_table_name(typeid(typename T::object_type)) << "' ";
                return ss.str();
            }
        };

        /**
         * This class represent a SQLite trigger
         // *  St is the statement type
         // *  S is the list of trigger statments
         */
        template<class St, class... S>
        struct trigger_t {
            using object_type = void;  // Not sure
            using table_type = typename St::object_type;
            using elements_type = typename St::elements_type;
            using statements_type = std::tuple<S...>;

            // Construct triggers by using one of the make_trigger functions
            trigger_t(std::string name,
                      trigger::trigger_timing_t timing,
                      St type,
                      bool for_each_row,
                      std::shared_ptr<void *> when,
                      S... statements) :
                name(std::move(name)),
                table(type.table), elements(type.table.elements), timing(timing), statement_type(type),
                for_each_row(for_each_row), when_expr(std::move(when)),
                stmts(std::tuple<S...>(std::forward<S>(statements)...)) {}

            /**
             * Name of the trigger
             */
            std::string name;

            /**
             * Value indicating if the timer is run BEFORE, AFTER or INSTEAD OF the
             * statement that fired it.
             * Defaults to BEFORE in the SQLite documentation
             */
            trigger::trigger_timing_t timing = trigger::trigger_before;

            /**
             * The type of the statement that would cause the trigger to trigger.
             * Can be DELETE, INSERT, or UPDATE.
             * There is no default for this parameter, it must be specified.
             */
            St statement_type;

            /**
             * Table the trigger is watching
             */
            typename St::table_type table;

            elements_type &elements;

            /**
             * Value used to determine if we execute the trigger on each row or on each statement
             * (SQLite doesn't support the FOR EACH STATEMENT syntax yet: https://sqlite.org/lang_createtrigger.html#description
             * so this value is more of a placeholder for a later update)
             * Defaults to true in SQLite documentation
             */
            bool for_each_row = true;

            /**
             * When expression (if any)
             * If a when expression is specified, the trigger will only execute
             * if the expression evaluates to true when the trigger is fired
             */
            std::shared_ptr<void *> when_expr = nullptr;

            /**
             * Statements of the triggers (to be executed when the trigger fires)
             */
            statements_type stmts;

            // Noop implementations, taken from `foreign_key_t`
            template<class L>
            void for_each_column(const L &_) {}

            template<class... Opts>
            constexpr bool has_every() const {
                return false;
            }
        };
    }  // NAMESPACE internal

    /*
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, statements...)
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, FOR_EACH_ROW, statements...)
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, WHEN, statements...)
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, FOR_EACH_ROW, WHEN, statements...)
     */

    namespace trigger {
        template<class T>
        inline internal::on_delete_t<T> on_delete(T table) {
            return {std::forward<T>(table)};
        }

        template<class T>
        inline internal::on_insert_t<T> on_insert(T table) {
            return {std::forward<T>(table)};
        }

        template<class T, class... Cs>
        inline internal::on_update_t<T, Cs...> on_update(T table, Cs... cols) {
            return {std::forward<T>(table), std::forward<Cs>(cols)...};
        }
    }

    template<class St, class... S>
    internal::trigger_t<St, S...>
    make_trigger(const std::string &name, trigger::trigger_timing_t timing, St type, S... statements) {
        return {name, timing, type, true, nullptr, std::forward<S>(statements)...};
    }
    // NOTE Disabled because for_each_row is a placeholder for now, so overloads to set it are useless
    // template<class St, class... S>
    // internal::trigger_t<T, St, S...> make_trigger(const std::string &name, trigger::trigger_timing_t timing, St type, bool for_each_row, S... statements) {
    //     return internal::trigger_t<T, St, S...>(name, timing, type, for_each_row, nullptr, statements...);
    // }
    template<class St, class... S>
    internal::trigger_t<St, S...> make_trigger(const std::string &name,
                                               trigger::trigger_timing_t timing,
                                               St type,
                                               std::shared_ptr<void *> when,
                                               S... statements) {
        return {name, timing, type, true, when, std::forward<S>(statements)...};
    }
    // NOTE Disabled because for_each_row is a placeholder for now, so overloads to set it are useless
    // template<class St, class... S>
    // internal::trigger_t<T, St, S...> make_trigger(const std::string &name, trigger::trigger_timing_t timing, St type, bool for_each_row, std::shared_ptr<void *> when, S... statements) {
    //     return internal::trigger_t<T, St, S...>(name, timing, type, for_each_row, when, statements...);
    // };
}
