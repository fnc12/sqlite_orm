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
    namespace internal {
        enum class trigger_timing { trigger_before, trigger_after, trigger_instead_of };
        enum class trigger_type { trigger_delete, trigger_insert, trigger_update };

        /**
         * This class is an intermediate SQLite trigger, to be used with
         * `make_trigger` to create a full trigger.
         *  T is the base of the trigger (contains its type, timing and associated table)
         *  S is the list of trigger statements
         */
        template<class T, class... S>
        struct partial_trigger_t {
            using statements_type = std::tuple<S...>;

            /**
             * Statements of the triggers (to be executed when the trigger fires)
             */
            statements_type statements;
            /**
             * Base of the trigger (contains its type, timing and associated table)
             */
            T base;

            partial_trigger_t(T trigger_base, S... statements) :
                statements(std::make_tuple<S...>(std::forward<S>(statements)...)), base(std::move(trigger_base)) {}

            partial_trigger_t &end() {
                return *this;
            }
        };

        /**
         * This class represent a SQLite trigger
         *  T is the base of the trigger (contains its type, timing and associated table)
         *  S is the list of trigger statments
         */
        template<class T, class... S>
        struct trigger_t {
            using object_type = void;  // Not sure
            using elements_type = typename partial_trigger_t<T, S...>::statements_type;

            /**
             * Name of the trigger
             */
            std::string name;

            /**
             * Base of the trigger (contains its type, timing and associated table)
             */
            T base;

            /**
             * Statements of the triggers (to be executed when the trigger fires)
             */
            elements_type elements;

            trigger_t(const std::string &name, T trigger_base, elements_type statements) :
                name(name), base(std::move(trigger_base)), elements(std::move(statements)) {}
        };

        /**
         * Base of a trigger. Contains the trigger type/timming and the table type
         * T is the table type
         * Type is the trigger base type (type+timing)
         */
        template<class T, class Type>
        struct trigger_base_t {
            using table_type = T;
            using trigger_type_base = Type;

            /**
             * Contains the trigger type and timing
             */
            trigger_type_base type_base;
            /**
             * Value used to determine if we execute the trigger on each row or on each statement
             * (SQLite doesn't support the FOR EACH STATEMENT syntax yet: https://sqlite.org/lang_createtrigger.html#description
             * so this value is more of a placeholder for a later update)
             * Defaults to true in SQLite documentation
             */
            bool do_for_each_row = true;
            /**
             * When expression (if any)
             * If a WHEN expression is specified, the trigger will only execute
             * if the expression evaluates to true when the trigger is fired
             */
            // void when = NOT_IMPLEMENTED;

            trigger_base_t(trigger_type_base type_base) : type_base(std::move(type_base)) {}

            trigger_base_t &for_each_row() {
                this->do_for_each_row = true;
                return *this;
            }
            // trigger_base_t &when(void); // TODO

            template<class... S>
            partial_trigger_t<trigger_base_t<T, Type>, S...> begin(S... statements) {
                return {*this, std::forward<S>(statements)...};
            }
        };

        /**
         * Contains the trigger type and timing
         */
        struct trigger_type_base_t {
            /**
             * Value indicating if the trigger is run BEFORE, AFTER or INSTEAD OF
             * the statement that fired it.
             */
            trigger_timing timing;
            /**
             * The type of the statement that would cause the trigger to fire.
             * Can be DELETE, INSERT, or UPDATE.
             */
            trigger_type type;

            trigger_type_base_t(trigger_timing timing, trigger_type type) : timing(timing), type(type) {}

            template<class T>
            trigger_base_t<T, trigger_type_base_t> on() {
                return {*this};
            }
        };

        /**
         * Special case for UPDATE OF (columns)
         * Contains the trigger type and timing
         */
        template<class... Cs>
        struct trigger_update_type_t : trigger_type_base_t {
            using columns_type = std::tuple<Cs...>;

            /**
             * Contains the columns the trigger is watching. Will only
             * trigger if one of theses columns is updated.
             */
            columns_type columns;

            trigger_update_type_t(trigger_timing timing, trigger_type type, Cs... columns) :
                trigger_type_base_t(timing, type), columns(std::make_tuple<Cs...>(std::forward<Cs>(columns)...)) {}

            template<class T>
            trigger_base_t<T, trigger_update_type_t<Cs...>> on() {
                return {*this};
            }
        };

        struct trigger_timing_t {
            trigger_timing timing;

            trigger_timing_t(trigger_timing timing) : timing(timing) {}

            trigger_type_base_t delete_() {
                return {timing, trigger_type::trigger_delete};
            }

            trigger_type_base_t insert() {
                return {timing, trigger_type::trigger_insert};
            }

            trigger_type_base_t update() {
                return {timing, trigger_type::trigger_update};
            }

            template<class... Cs>
            trigger_update_type_t<Cs...> update_of(Cs... columns) {
                return {timing, trigger_type::trigger_update, std::forward<Cs>(columns)...};
            }
        };
    }  // NAMESPACE internal

    template<class T, class... S>
    internal::trigger_t<T, S...> make_trigger(const std::string &name,
                                              const internal::partial_trigger_t<T, S...> &part) {
        return {name, std::move(part.base), std::move(part.statements)};
    }

    inline internal::trigger_timing_t before() {
        return {internal::trigger_timing_t(internal::trigger_timing::trigger_before)};
    }

    inline internal::trigger_timing_t after() {
        return {internal::trigger_timing_t(internal::trigger_timing::trigger_after)};
    }

    inline internal::trigger_timing_t instead_of() {
        return {internal::trigger_timing_t(internal::trigger_timing::trigger_instead_of)};
    }
}
