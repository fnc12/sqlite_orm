#pragma once

#include <system_error>  //  std::system_error
#include <ostream>  //  std::ostream
#include <string>  //  std::string
#include <tuple>  //  std::tuple, std::make_tuple
#include <type_traits>  //  std::is_base_of, std::false_type, std::true_type

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "functional/mpl.h"
#include "tuple_helper/same_or_void.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "type_traits.h"
#include "collate_argument.h"
#include "error_code.h"
#include "table_type_of.h"
#include "type_printer.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  AUTOINCREMENT constraint class.
         */
        struct autoincrement_t {};

        enum class conflict_clause_t {
            rollback,
            abort,
            fail,
            ignore,
            replace,
        };

        struct primary_key_base {
            enum class order_by {
                unspecified,
                ascending,
                descending,
            };
            struct {
                order_by asc_option = order_by::unspecified;
                conflict_clause_t conflict_clause = conflict_clause_t::rollback;
                bool conflict_clause_is_on = false;
            } options;
        };

        template<class T>
        struct primary_key_with_autoincrement {
            using primary_key_type = T;

            primary_key_type primary_key;

            primary_key_with_autoincrement(primary_key_type primary_key_) : primary_key(primary_key_) {}
        };

        /**
         *  PRIMARY KEY constraint class.
         *  Cs is parameter pack which contains columns (member pointers and/or function pointers). Can be empty when
         *  used within `make_column` function.
         */
        template<class... Cs>
        struct primary_key_t : primary_key_base {
            using self = primary_key_t<Cs...>;
            using order_by = primary_key_base::order_by;
            using columns_tuple = std::tuple<Cs...>;

            columns_tuple columns;

            primary_key_t(decltype(columns) columns) : columns(std::move(columns)) {}

            self asc() const {
                auto res = *this;
                res.options.asc_option = order_by::ascending;
                return res;
            }

            self desc() const {
                auto res = *this;
                res.options.asc_option = order_by::descending;
                return res;
            }

            primary_key_with_autoincrement<self> autoincrement() const {
                return {*this};
            }

            self on_conflict_rollback() const {
                auto res = *this;
                res.options.conflict_clause_is_on = true;
                res.options.conflict_clause = conflict_clause_t::rollback;
                return res;
            }

            self on_conflict_abort() const {
                auto res = *this;
                res.options.conflict_clause_is_on = true;
                res.options.conflict_clause = conflict_clause_t::abort;
                return res;
            }

            self on_conflict_fail() const {
                auto res = *this;
                res.options.conflict_clause_is_on = true;
                res.options.conflict_clause = conflict_clause_t::fail;
                return res;
            }

            self on_conflict_ignore() const {
                auto res = *this;
                res.options.conflict_clause_is_on = true;
                res.options.conflict_clause = conflict_clause_t::ignore;
                return res;
            }

            self on_conflict_replace() const {
                auto res = *this;
                res.options.conflict_clause_is_on = true;
                res.options.conflict_clause = conflict_clause_t::replace;
                return res;
            }
        };

        struct unique_base {
            operator std::string() const {
                return "UNIQUE";
            }
        };

        /**
         *  UNIQUE constraint class.
         */
        template<class... Args>
        struct unique_t : unique_base {
            using columns_tuple = std::tuple<Args...>;

            columns_tuple columns;

            unique_t(columns_tuple columns_) : columns(std::move(columns_)) {}
        };

        /**
         *  DEFAULT constraint class.
         *  T is a value type.
         */
        template<class T>
        struct default_t {
            using value_type = T;

            value_type value;

            operator std::string() const {
                return "DEFAULT";
            }
        };

#if SQLITE_VERSION_NUMBER >= 3006019

        /**
         *  FOREIGN KEY constraint class.
         *  Cs are columns which has foreign key
         *  Rs are column which C references to
         *  Available in SQLite 3.6.19 or higher
         */

        template<class A, class B>
        struct foreign_key_t;

        enum class foreign_key_action {
            none,  //  not specified
            no_action,
            restrict_,
            set_null,
            set_default,
            cascade,
        };

        inline std::ostream& operator<<(std::ostream& os, foreign_key_action action) {
            switch(action) {
                case foreign_key_action::no_action:
                    os << "NO ACTION";
                    break;
                case foreign_key_action::restrict_:
                    os << "RESTRICT";
                    break;
                case foreign_key_action::set_null:
                    os << "SET NULL";
                    break;
                case foreign_key_action::set_default:
                    os << "SET DEFAULT";
                    break;
                case foreign_key_action::cascade:
                    os << "CASCADE";
                    break;
                case foreign_key_action::none:
                    break;
            }
            return os;
        }

        struct on_update_delete_base {
            const bool update;  //  true if update and false if delete

            operator std::string() const {
                if(this->update) {
                    return "ON UPDATE";
                } else {
                    return "ON DELETE";
                }
            }
        };

        /**
         *  F - foreign key class
         */
        template<class F>
        struct on_update_delete_t : on_update_delete_base {
            using foreign_key_type = F;

            const foreign_key_type& fk;

            on_update_delete_t(decltype(fk) fk_, decltype(update) update_, foreign_key_action action_) :
                on_update_delete_base{update_}, fk(fk_), _action(action_) {}

            foreign_key_action _action = foreign_key_action::none;

            foreign_key_type no_action() const {
                auto res = this->fk;
                if(update) {
                    res.on_update._action = foreign_key_action::no_action;
                } else {
                    res.on_delete._action = foreign_key_action::no_action;
                }
                return res;
            }

            foreign_key_type restrict_() const {
                auto res = this->fk;
                if(update) {
                    res.on_update._action = foreign_key_action::restrict_;
                } else {
                    res.on_delete._action = foreign_key_action::restrict_;
                }
                return res;
            }

            foreign_key_type set_null() const {
                auto res = this->fk;
                if(update) {
                    res.on_update._action = foreign_key_action::set_null;
                } else {
                    res.on_delete._action = foreign_key_action::set_null;
                }
                return res;
            }

            foreign_key_type set_default() const {
                auto res = this->fk;
                if(update) {
                    res.on_update._action = foreign_key_action::set_default;
                } else {
                    res.on_delete._action = foreign_key_action::set_default;
                }
                return res;
            }

            foreign_key_type cascade() const {
                auto res = this->fk;
                if(update) {
                    res.on_update._action = foreign_key_action::cascade;
                } else {
                    res.on_delete._action = foreign_key_action::cascade;
                }
                return res;
            }

            operator bool() const {
                return this->_action != foreign_key_action::none;
            }
        };

        template<class F>
        bool operator==(const on_update_delete_t<F>& lhs, const on_update_delete_t<F>& rhs) {
            return lhs._action == rhs._action;
        }

        template<class... Cs, class... Rs>
        struct foreign_key_t<std::tuple<Cs...>, std::tuple<Rs...>> {
            using columns_type = std::tuple<Cs...>;
            using references_type = std::tuple<Rs...>;
            using self = foreign_key_t<columns_type, references_type>;

            /**
             * Holds obect type of all referenced columns.
             */
            using target_type = typename same_or_void<table_type_of_t<Rs>...>::type;

            /**
             * Holds obect type of all source columns.
             */
            using source_type = typename same_or_void<table_type_of_t<Cs>...>::type;

            columns_type columns;
            references_type references;

            on_update_delete_t<self> on_update;
            on_update_delete_t<self> on_delete;

            static_assert(std::tuple_size<columns_type>::value == std::tuple_size<references_type>::value,
                          "Columns size must be equal to references tuple");
            static_assert(!std::is_same<target_type, void>::value, "All references must have the same type");

            foreign_key_t(columns_type columns_, references_type references_) :
                columns(std::move(columns_)), references(std::move(references_)),
                on_update(*this, true, foreign_key_action::none), on_delete(*this, false, foreign_key_action::none) {}

            foreign_key_t(const self& other) :
                columns(other.columns), references(other.references), on_update(*this, true, other.on_update._action),
                on_delete(*this, false, other.on_delete._action) {}

            self& operator=(const self& other) {
                this->columns = other.columns;
                this->references = other.references;
                this->on_update = {*this, true, other.on_update._action};
                this->on_delete = {*this, false, other.on_delete._action};
                return *this;
            }
        };

        template<class A, class B>
        bool operator==(const foreign_key_t<A, B>& lhs, const foreign_key_t<A, B>& rhs) {
            return lhs.columns == rhs.columns && lhs.references == rhs.references && lhs.on_update == rhs.on_update &&
                   lhs.on_delete == rhs.on_delete;
        }

        /**
         *  Cs can be a class member pointer, a getter function member pointer or setter
         *  func member pointer
         *  Available in SQLite 3.6.19 or higher
         */
        template<class... Cs>
        struct foreign_key_intermediate_t {
            using tuple_type = std::tuple<Cs...>;

            tuple_type columns;

            template<class... Rs>
            foreign_key_t<std::tuple<Cs...>, std::tuple<Rs...>> references(Rs... refs) {
                return {std::move(this->columns), std::make_tuple(std::forward<Rs>(refs)...)};
            }
        };
#endif

        struct collate_constraint_t {
            collate_argument argument = collate_argument::binary;
#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            collate_constraint_t(collate_argument argument) : argument{argument} {}
#endif

            operator std::string() const {
                return "COLLATE " + this->string_from_collate_argument(this->argument);
            }

            static std::string string_from_collate_argument(collate_argument argument) {
                switch(argument) {
                    case collate_argument::binary:
                        return "BINARY";
                    case collate_argument::nocase:
                        return "NOCASE";
                    case collate_argument::rtrim:
                        return "RTRIM";
                }
                throw std::system_error{orm_error_code::invalid_collate_argument_enum};
            }
        };

        template<class T>
        struct check_t {
            using expression_type = T;

            expression_type expression;
        };

        struct basic_generated_always {
            enum class storage_type {
                not_specified,
                virtual_,
                stored,
            };

            bool full = true;
            storage_type storage = storage_type::not_specified;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            basic_generated_always(bool full, storage_type storage) : full{full}, storage{storage} {}
#endif
        };

        template<class T>
        struct generated_always_t : basic_generated_always {
            using expression_type = T;

            expression_type expression;

            generated_always_t(expression_type expression_, bool full, storage_type storage) :
                basic_generated_always{full, storage}, expression(std::move(expression_)) {}

            generated_always_t<T> virtual_() {
                return {std::move(this->expression), this->full, storage_type::virtual_};
            }

            generated_always_t<T> stored() {
                return {std::move(this->expression), this->full, storage_type::stored};
            }
        };

    }

    namespace internal {

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_foreign_key_v = polyfill::is_specialization_of_v<T, foreign_key_t>;

        template<class T>
        using is_foreign_key = polyfill::bool_constant<is_foreign_key_v<T>>;

        template<class T>
        struct is_primary_key : std::false_type {};

        template<class... Cs>
        struct is_primary_key<primary_key_t<Cs...>> : std::true_type {};

        template<class T>
        struct is_primary_key<primary_key_with_autoincrement<T>> : std::true_type {};

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_primary_key_v = is_primary_key<T>::value;

        template<class T>
        using is_generated_always = polyfill::is_specialization_of<T, generated_always_t>;

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_generated_always_v = is_generated_always<T>::value;

        template<class T>
        using is_autoincrement = std::is_same<T, autoincrement_t>;

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_autoincrement_v = is_autoincrement<T>::value;

        /**
         * PRIMARY KEY INSERTABLE traits.
         */
        template<typename T>
        struct is_primary_key_insertable
            : polyfill::disjunction<
                  mpl::instantiate<mpl::disjunction<check_if_tuple_has<is_autoincrement>,
                                                    check_if_tuple_has_template<default_t>,
                                                    check_if_tuple_has_template<primary_key_with_autoincrement>>,
                                   constraints_type_t<T>>,
                  std::is_base_of<integer_printer, type_printer<field_type_t<T>>>> {

            static_assert(tuple_has<is_primary_key, constraints_type_t<T>>::value, "an unexpected type was passed");
        };

        template<class T>
        using is_constraint =
            mpl::instantiate<mpl::disjunction<check_if<is_autoincrement>,
                                              check_if<is_primary_key>,
                                              check_if<is_foreign_key>,
                                              check_if_is_template<unique_t>,
                                              check_if_is_template<default_t>,
                                              check_if_is_template<check_t>,
                                              check_if_is_template<primary_key_with_autoincrement>,
                                              check_if_is_type<collate_constraint_t>,
#if SQLITE_VERSION_NUMBER >= 3031000
                                              check_if<is_generated_always>,
#endif
                                              // dummy tail because of SQLITE_VERSION_NUMBER checks above
                                              mpl::always<std::false_type>>,
                             T>;
    }

#if SQLITE_VERSION_NUMBER >= 3031000
    template<class T>
    internal::generated_always_t<T> generated_always_as(T expression) {
        return {std::move(expression), true, internal::basic_generated_always::storage_type::not_specified};
    }

    template<class T>
    internal::generated_always_t<T> as(T expression) {
        return {std::move(expression), false, internal::basic_generated_always::storage_type::not_specified};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3006019

    /**
     *  FOREIGN KEY constraint construction function that takes member pointer as argument
     *  Available in SQLite 3.6.19 or higher
     */
    template<class... Cs>
    internal::foreign_key_intermediate_t<Cs...> foreign_key(Cs... columns) {
        return {std::make_tuple(std::forward<Cs>(columns)...)};
    }
#endif

    /**
     *  UNIQUE constraint builder function.
     */
    template<class... Args>
    internal::unique_t<Args...> unique(Args... args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    inline internal::unique_t<> unique() {
        return {{}};
    }

    /**
     *  AUTOINCREMENT keyword. [Deprecation notice] Use `primary_key().autoincrement()` instead of using this function.
     *  This function will be removed in 1.9
     */
    [[deprecated("Use primary_key().autoincrement()` instead")]] inline internal::autoincrement_t autoincrement() {
        return {};
    }

    template<class... Cs>
    internal::primary_key_t<Cs...> primary_key(Cs... cs) {
        return {std::make_tuple(std::forward<Cs>(cs)...)};
    }

    inline internal::primary_key_t<> primary_key() {
        return {{}};
    }

    template<class T>
    internal::default_t<T> default_value(T t) {
        return {std::move(t)};
    }

    inline internal::collate_constraint_t collate_nocase() {
        return {internal::collate_argument::nocase};
    }

    inline internal::collate_constraint_t collate_binary() {
        return {internal::collate_argument::binary};
    }

    inline internal::collate_constraint_t collate_rtrim() {
        return {internal::collate_argument::rtrim};
    }

    template<class T>
    internal::check_t<T> check(T t) {
        return {std::move(t)};
    }
}
