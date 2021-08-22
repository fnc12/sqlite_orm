#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <tuple>

#include "tuple_helper.h"

// NOTE Idea : Maybe also implement a custom trigger system to call a c++ callback when a trigger triggers ?
// (Could be implemented with a normal trigger that insert or update an internal table and then retreive
// the event in the C++ code, to call the C++ user callback, with update hooks: https://www.sqlite.org/c3ref/update_hook.html)
// It could be an interesting feature to bring to sqlite_orm that other libraries don't have ?

namespace sqlite_orm {
    namespace trigger {
        enum trigger_timing_t {
            trigger_before,
            trigger_after,
            trigger_instead_of
        };

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
        struct trigger_statement_type_base_t {
            // Disallow users to instantiate this class
            protected:
                trigger_statement_type_base_t() = default;
        };

        struct on_delete_t : trigger_statement_type_base_t {
            operator std::string() const {
                return "DELETE";
            }
        };

        struct on_insert_t : trigger_statement_type_base_t {
            operator std::string() const {
                return "INSERT";
            }
        };

        template<class... Cs>
        struct on_update_t : trigger_statement_type_base_t {
            using columns_type = std::tuple<Cs...>;

            columns_type columns;

            on_update_t(Cs... args) :
                columns(std::make_tuple<Cs...>(std::forward<Cs>(args)...))
            {}

            operator std::string() const {
                std::stringstream ss;
                std::string sep = " OF ";

                ss << "UPDATE";
                iterate_tuple(columns, [&ss, &sep](auto &node) {
                    ss << sep << node;
                    sep = ", ";
                });

                return ss.str();
            }
        };

        /**
         * This class represent a SQLite trigger
         // *  T is a mapped table class, e.g. User
         // *  C is a mapped column of this class
         */
        template<class T, class... S>
        struct trigger_t {
            using statements_type = std::tuple<S...>;

            // Construct triggers by using one of the make_trigger functions
            trigger_t(std::string &name, trigger::trigger_timing_t timing, trigger_statement_type_base_t &type, T table, bool for_each_row, std::shared_ptr<void *> when, S... statements) :
                name(std::move(name)), table(table), timing(timing),
                statement_type(std::make_unique<trigger_statement_type_base_t>(type)),
                for_each_row(for_each_row), when_expr(std::move(when)),
                stmts(std::tuple<S...>(std::forward<S>(statements)...))
            {}

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
            std::unique_ptr<internal::trigger_statement_type_base_t> statement_type;

            /**
             * Table the trigger is watching
             */
            T table;

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

            // Noop implementations, copied from `foreign_key_t`
            template<class L>
            void for_each_column(const L &_) {}

            template<class... Opts>
            constexpr bool has_every() const {
                return false;
            }
        };
    } // NAMESPACE internal

    /*
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, statements...)
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, FOR_EACH_ROW, statements...)
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, WHEN, statements...)
      make_trigger(name, BEFORE|AFTER|INSTEAD_OF, DELETE|INSERT|UPDATE([COLUMN_NAME,...]), table, FOR_EACH_ROW, WHEN, statements...)
     */

    namespace trigger {
        inline internal::on_delete_t on_delete()
        { return {}; }

        inline internal::on_insert_t on_insert()
        { return {}; }

        template<class... Cs>
        inline internal::on_update_t<Cs...> on_update(Cs... cols)
        { return {std::forward<Cs>(cols)...}; }
    }

    template<class T, class... S>
    internal::trigger_t<S...> make_trigger(const std::string &name, trigger::trigger_timing_t timing, internal::trigger_statement_type_base_t &&type, T table, S... statements) {
        return make_trigger(name, timing, type, table, true, nullptr, statements...);
    }
    template<class T, class... S>
    internal::trigger_t<S...> make_trigger(const std::string &name, trigger::trigger_timing_t timing, internal::trigger_statement_type_base_t &&type, T table, bool for_each_row, S... statements) {
        return make_trigger(name, timing, type, table, for_each_row, nullptr, statements...);
    }
    template<class T, class... S>
    internal::trigger_t<S...> make_trigger(const std::string &name, trigger::trigger_timing_t timing, internal::trigger_statement_type_base_t &&type, T table, std::shared_ptr<void *> when, S... statements) {
        return make_trigger(name, timing, type, table, true, when, statements...);
    }
    template<class T, class... S>
    internal::trigger_t<S...> make_trigger(const std::string &name, trigger::trigger_timing_t timing, internal::trigger_statement_type_base_t &&type, T table, bool for_each_row, std::shared_ptr<void *> when, S... statements);
}
