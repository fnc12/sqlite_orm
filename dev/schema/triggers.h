#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <tuple>

#include "functional/cxx_universal.h"
#include "optional_container.h"

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
             * Base of the trigger (contains its type, timing and associated table)
             */
            T base;
            /**
             * Statements of the triggers (to be executed when the trigger fires)
             */
            statements_type statements;

            partial_trigger_t(T trigger_base, S... statements) :
                base{std::move(trigger_base)}, statements{std::make_tuple<S...>(std::forward<S>(statements)...)} {}

            partial_trigger_t& end() {
                return *this;
            }
        };

        struct base_trigger {
            /**
             * Name of the trigger
             */
            std::string name;
        };

        /**
         * This class represent a SQLite trigger
         *  T is the base of the trigger (contains its type, timing and associated table)
         *  S is the list of trigger statments
         */
        template<class T, class... S>
        struct trigger_t : base_trigger {
            using object_type = void;
            using elements_type = typename partial_trigger_t<T, S...>::statements_type;

            /**
             * Base of the trigger (contains its type, timing and associated table)
             */
            T base;

            /**
             * Statements of the triggers (to be executed when the trigger fires)
             */
            elements_type elements;

#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            trigger_t(std::string name, T trigger_base, elements_type statements) :
                base_trigger{std::move(name)}, base(std::move(trigger_base)), elements(std::move(statements)) {}
#endif
        };

        /**
         * Base of a trigger. Contains the trigger type/timming and the table type
         * T is the table type
         * W is `when` expression type
         * Type is the trigger base type (type+timing)
         */
        template<class T, class W, class Type>
        struct trigger_base_t {
            using table_type = T;
            using when_type = W;
            using trigger_type_base = Type;

            /**
             * Contains the trigger type and timing
             */
            trigger_type_base type_base;
            /**
             * Value used to determine if we execute the trigger on each row or on each statement
             * (SQLite doesn't support the FOR EACH STATEMENT syntax yet: https://sqlite.org/lang_createtrigger.html#description
             * so this value is more of a placeholder for a later update)
             */
            bool do_for_each_row = false;
            /**
             * When expression (if any)
             * If a WHEN expression is specified, the trigger will only execute
             * if the expression evaluates to true when the trigger is fired
             */
            optional_container<when_type> container_when;

            trigger_base_t(trigger_type_base type_base_) : type_base(std::move(type_base_)) {}

            trigger_base_t& for_each_row() {
                this->do_for_each_row = true;
                return *this;
            }

            template<class WW>
            trigger_base_t<T, WW, Type> when(WW expression) {
                trigger_base_t<T, WW, Type> res(this->type_base);
                res.container_when.field = std::move(expression);
                return res;
            }

            template<class... S>
            partial_trigger_t<trigger_base_t<T, W, Type>, S...> begin(S... statements) {
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
            trigger_base_t<T, void, trigger_type_base_t> on() {
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
            trigger_base_t<T, void, trigger_update_type_t<Cs...>> on() {
                return {*this};
            }
        };

        struct trigger_timing_t {
            trigger_timing timing;

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

        struct raise_t {
            enum class type_t {
                ignore,
                rollback,
                abort,
                fail,
            };

            type_t type = type_t::ignore;
            std::string message;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            raise_t(type_t type, std::string message) : type{type}, message{std::move(message)} {}
#endif
        };

        template<class T>
        struct new_t {
            using expression_type = T;

            expression_type expression;
        };

        template<class T>
        struct old_t {
            using expression_type = T;

            expression_type expression;
        };
    }  // NAMESPACE internal

    /**
     *  NEW.expression function used within TRIGGER expressions
     */
    template<class T>
    internal::new_t<T> new_(T expression) {
        return {std::move(expression)};
    }

    /**
     *  OLD.expression function used within TRIGGER expressions
     */
    template<class T>
    internal::old_t<T> old(T expression) {
        return {std::move(expression)};
    }

    /**
     *  RAISE(IGNORE) expression used within TRIGGER expressions
     */
    inline internal::raise_t raise_ignore() {
        return {internal::raise_t::type_t::ignore, {}};
    }

    /**
     *  RAISE(ROLLBACK, %message%) expression used within TRIGGER expressions
     */
    inline internal::raise_t raise_rollback(std::string message) {
        return {internal::raise_t::type_t::rollback, std::move(message)};
    }

    /**
     *  RAISE(ABORT, %message%) expression used within TRIGGER expressions
     */
    inline internal::raise_t raise_abort(std::string message) {
        return {internal::raise_t::type_t::abort, std::move(message)};
    }

    /**
     *  RAISE(FAIL, %message%) expression used within TRIGGER expressions
     */
    inline internal::raise_t raise_fail(std::string message) {
        return {internal::raise_t::type_t::fail, std::move(message)};
    }

    template<class T, class... S>
    internal::trigger_t<T, S...> make_trigger(std::string name, const internal::partial_trigger_t<T, S...>& part) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), std::move(part.base), std::move(part.statements)});
    }

    inline internal::trigger_timing_t before() {
        return {internal::trigger_timing::trigger_before};
    }

    inline internal::trigger_timing_t after() {
        return {internal::trigger_timing::trigger_after};
    }

    inline internal::trigger_timing_t instead_of() {
        return {internal::trigger_timing::trigger_instead_of};
    }
}
