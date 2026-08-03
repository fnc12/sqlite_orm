#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of, std::false_type, std::true_type
#include <system_error>  //  std::system_error
#include <ostream>  //  std::ostream
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/mpl.h"
#include "tuple_helper/same_or_void.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "type_traits.h"
#include "collate_argument.h"
#include "error_code.h"
#include "field_of.h"
#include "table_type_of.h"
#include "type_printer.h"

namespace sqlite_orm::internal {
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
    struct primary_key_with_autoincrement : T {
        using primary_key_type = T;

        constexpr const primary_key_type& as_base() const {
            return *this;
        }
    };

    /**
     *  PRIMARY KEY constraint class.
     *  Cs is parameter pack which contains columns (member pointers and/or function pointers). Can be empty when
     *  used within `make_column` function.
     */
    template<class... Cs>
    struct primary_key_t : primary_key_base {
        using order_by = primary_key_base::order_by;
        using columns_tuple = std::tuple<Cs...>;

        columns_tuple columns;

        constexpr primary_key_t(columns_tuple columns) : columns(std::move(columns)) {}

        constexpr primary_key_t asc() const {
            auto res = *this;
            res.options.asc_option = order_by::ascending;
            return res;
        }

        constexpr primary_key_t desc() const {
            auto res = *this;
            res.options.asc_option = order_by::descending;
            return res;
        }

        constexpr primary_key_with_autoincrement<primary_key_t> autoincrement() const {
            return {*this};
        }

        constexpr primary_key_t on_conflict_rollback() const {
            auto res = *this;
            res.options.conflict_clause_is_on = true;
            res.options.conflict_clause = conflict_clause_t::rollback;
            return res;
        }

        constexpr primary_key_t on_conflict_abort() const {
            auto res = *this;
            res.options.conflict_clause_is_on = true;
            res.options.conflict_clause = conflict_clause_t::abort;
            return res;
        }

        constexpr primary_key_t on_conflict_fail() const {
            auto res = *this;
            res.options.conflict_clause_is_on = true;
            res.options.conflict_clause = conflict_clause_t::fail;
            return res;
        }

        constexpr primary_key_t on_conflict_ignore() const {
            auto res = *this;
            res.options.conflict_clause_is_on = true;
            res.options.conflict_clause = conflict_clause_t::ignore;
            return res;
        }

        constexpr primary_key_t on_conflict_replace() const {
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

        constexpr unique_t(columns_tuple columns_) : columns(std::move(columns_)) {}
    };

    struct unindexed_t {};

    template<class T>
    struct prefix_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    struct tokenize_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    struct content_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    struct table_content_t {
        using mapped_type = T;
    };

#if SQLITE_VERSION_NUMBER >= 3024000
    /** 
     *  Auxiliary virtual table column constraint
     */
    struct auxiliary_t {};

    template<class T>
    using is_auxiliary = std::is_same<T, auxiliary_t>;
#else
    template<class T>
    using is_auxiliary = std::false_type;
#endif

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
     *  Available since SQLite 3.6.19
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
        switch (action) {
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
            if (this->update) {
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

        foreign_key_action _action = foreign_key_action::none;

        on_update_delete_t(foreign_key_type& fk_, decltype(update) update_, foreign_key_action action_) :
            on_update_delete_base{update_}, fk(fk_), _action(action_) {}

        foreign_key_type no_action() const {
            auto res = this->fk;
            if (update) {
                res.on_update._action = foreign_key_action::no_action;
            } else {
                res.on_delete._action = foreign_key_action::no_action;
            }
            return res;
        }

        foreign_key_type restrict_() const {
            auto res = this->fk;
            if (update) {
                res.on_update._action = foreign_key_action::restrict_;
            } else {
                res.on_delete._action = foreign_key_action::restrict_;
            }
            return res;
        }

        foreign_key_type set_null() const {
            auto res = this->fk;
            if (update) {
                res.on_update._action = foreign_key_action::set_null;
            } else {
                res.on_delete._action = foreign_key_action::set_null;
            }
            return res;
        }

        foreign_key_type set_default() const {
            auto res = this->fk;
            if (update) {
                res.on_update._action = foreign_key_action::set_default;
            } else {
                res.on_delete._action = foreign_key_action::set_default;
            }
            return res;
        }

        foreign_key_type cascade() const {
            auto res = this->fk;
            if (update) {
                res.on_update._action = foreign_key_action::cascade;
            } else {
                res.on_delete._action = foreign_key_action::cascade;
            }
            return res;
        }

        explicit operator bool() const {
            return _action != foreign_key_action::none;
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

        /**
         *  Holds obect type of all referenced columns.
         */
        using target_type = same_or_void_t<table_type_of_t<Rs>...>;

        /**
         *  Holds obect type of all source columns.
         */
        using source_type = same_or_void_t<table_type_of_t<Cs>...>;

        columns_type _columns;
        references_type _references;

        on_update_delete_t<foreign_key_t> on_update;
        on_update_delete_t<foreign_key_t> on_delete;

        static_assert(std::tuple_size<columns_type>::value == std::tuple_size<references_type>::value,
                      "Columns size must be equal to references tuple");
        static_assert(!std::is_same<source_type, void>::value, "All columns must have the same mapped type");
        static_assert(!std::is_same<target_type, void>::value, "All references must have the same mapped type");

        foreign_key_t(columns_type columns, references_type references) :
            _columns(std::move(columns)), _references(std::move(references)),
            on_update(*this, true, foreign_key_action::none), on_delete(*this, false, foreign_key_action::none) {}

        foreign_key_t(const foreign_key_t& other) :
            _columns(other._columns), _references(other._references), on_update(*this, true, other.on_update._action),
            on_delete(*this, false, other.on_delete._action) {}

        foreign_key_t& operator=(const foreign_key_t&) = delete;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        friend bool operator==(const foreign_key_t& lhs, const foreign_key_t& rhs) = default;
#else
        friend bool operator==(const foreign_key_t& lhs, const foreign_key_t& rhs) {
            return lhs._columns == rhs._columns && lhs._references == rhs._references &&
                   lhs.on_update == rhs.on_update && lhs.on_delete == rhs.on_delete;
        }
#endif
    };

    /**
     *  Cs can be a class member pointer or column pointer
     *  Available since SQLite 3.6.19
     */
    template<class... Cs>
    struct foreign_key_intermediate_t {
        using tuple_type = std::tuple<Cs...>;

        tuple_type _columns;

        /**
         *  Specify one or more target fields, which can either be pointers to class members or column pointers.
         */
        template<class... Rs>
        foreign_key_t<tuple_type, std::tuple<Rs...>> references(Rs... refs) {
            return {std::move(_columns), {std::forward<Rs>(refs)...}};
        }

        /**
         *  Specify one or more target fields that are member pointers of base classes,
         *  specifying the derived class as an explicit template argument.
         */
        template<class O, class... Base, class... F>
        foreign_key_t<tuple_type, std::tuple<F O::*...>> references(F Base::*... refs) {
            static_assert(polyfill::conjunction<is_field_of<F Base::*, O>...>::value,
                          "Referenced fields must be from explicitly specified derived class");
            return {std::move(_columns), {refs...}};
        }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /**
         *  Specify one or more target fields that are member pointers of base classes,
         *  specifying the derived class as an explicit template argument.
         */
        template<orm_table_reference auto table, class... Base, class... F>
        auto references(F Base::*... refs) {
            return this->references<auto_decay_table_ref_t<table>>(refs...);
        }
#endif
    };
#endif

    struct collate_constraint_t {
        collate_argument argument = collate_argument::binary;

        static std::string string_from_collate_argument(collate_argument argument) {
            switch (argument) {
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

#if SQLITE_VERSION_NUMBER >= 3031000
        bool full = true;
        storage_type storage = storage_type::not_specified;
#endif
    };

#if SQLITE_VERSION_NUMBER >= 3031000
    template<class T>
    struct generated_always_t : basic_generated_always {
        using expression_type = T;

        expression_type expression;

        constexpr generated_always_t(expression_type expression_, bool full, storage_type storage) :
            basic_generated_always{full, storage}, expression(std::move(expression_)) {}

        constexpr generated_always_t<T> virtual_() {
            return {std::move(this->expression), this->full, storage_type::virtual_};
        }

        constexpr generated_always_t<T> stored() {
            return {std::move(this->expression), this->full, storage_type::stored};
        }
    };
#endif

    struct null_t {};

    struct not_null_t {};

    template<class T>
    inline constexpr bool is_foreign_key_v =
#if SQLITE_VERSION_NUMBER >= 3006019
        polyfill::is_specialization_of<T, foreign_key_t>::value;
#else
        false;
#endif

    template<class T>
    struct is_foreign_key : polyfill::bool_constant<is_foreign_key_v<T>> {};

    template<class T>
    inline constexpr bool is_primary_key_v = std::is_base_of<primary_key_base, T>::value;

    template<class T>
    struct is_primary_key : polyfill::bool_constant<is_primary_key_v<T>> {};

    template<class T>
    inline constexpr bool is_generated_always_v =
#if SQLITE_VERSION_NUMBER >= 3031000
        polyfill::is_specialization_of<T, generated_always_t>::value;
#else
        false;
#endif

    template<class T>
    struct is_generated_always : polyfill::bool_constant<is_generated_always_v<T>> {};

    // Custom type:
    // It is the programmer's responsibility to ensure data integrity in the value range of the custom type
    // and in purview of SQLite using a 64-bit signed integer.
    template<class F, class SFINAE = void>
    inline constexpr bool is_rowid_alias_capable_v = std::is_base_of<integer_printer, type_printer<F>>::value;

    // For 64-bit signed integer type: capable
    template<class F>
    inline constexpr bool
        is_rowid_alias_capable_v<F,
                                 std::enable_if_t<std::is_integral<F>::value &&
                                                  (sizeof(F) == sizeof(sqlite_int64) &&
                                                   std::is_signed<F>::value == std::is_signed<sqlite_int64>::value)>> =
            true;

    // Design decision for integral types other than 64-bit signed integer:
    // It is the programmer's responsibility to ensure data integrity in the value range of the integral type
    // and in purview of SQLite using a 64-bit signed integer.
    template<class F>
    inline constexpr bool
        is_rowid_alias_capable_v<F,
                                 std::enable_if_t<std::is_integral<F>::value &&
                                                  (sizeof(F) != sizeof(sqlite_int64) ||
                                                   std::is_signed<F>::value != std::is_signed<sqlite_int64>::value)>> =
            true;

    // For non-integer types: unsuitable
    template<class F>
    inline constexpr bool
        is_rowid_alias_capable_v<F, std::enable_if_t<!std::is_base_of<integer_printer, type_printer<F>>::value>> =
            false;

    /** 
     *  COLUMN PRIMARY KEY INSERTABLE traits.
     *  
     *  A column primary key is considered implicitly insertable if:
     *  - it is an INTEGER PRIMARY KEY (and thus an alias for the "rowid" key),
     *  - or has a default value.
     *  
     *  In terms of C++ types, this means that the field type must be capable of representing a 64-bit signed integer,
     *  or the column is declared with a DEFAULT constraint.
     *  
     *  Implementation note: using a struct template in favor of a template alias so that the stack leading to a deprecation message is shorter.
     */
    template<typename Column>
    struct is_pkcol_implicitly_insertable
        : mpl::invoke_t<
              mpl::disjunction<mpl::always<polyfill::bool_constant<is_rowid_alias_capable_v<field_type_t<Column>>>>,
                               check_if_has_template<default_t>>,
              constraints_type_t<Column>> {};

    template<class T>
    using is_column_constraint = mpl::invoke_t<mpl::disjunction<check_if<std::is_base_of, primary_key_t<>>,
                                                                check_if_is_type<null_t>,
                                                                check_if_is_type<not_null_t>,
                                                                check_if_is_type<unique_t<>>,
                                                                check_if_is_template<default_t>,
                                                                check_if_is_template<check_t>,
                                                                check_if_is_type<collate_constraint_t>,
                                                                check_if<is_generated_always>,
                                                                check_if_is_type<unindexed_t>,
                                                                check_if<is_auxiliary>>,
                                               T>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3031000
    template<class T>
    constexpr internal::generated_always_t<T> generated_always_as(T expression) {
        return {std::move(expression), true, internal::basic_generated_always::storage_type::not_specified};
    }

    template<class T>
    constexpr internal::generated_always_t<T> as(T expression) {
        return {std::move(expression), false, internal::basic_generated_always::storage_type::not_specified};
    }
#endif

#if SQLITE_VERSION_NUMBER >= 3006019
    /**
     *  FOREIGN KEY constraint builder function taking one or more fields, which can either be pointers to class members or column pointers.
     *  Available since SQLite 3.6.19
     */
    template<class... Cs>
    constexpr internal::foreign_key_intermediate_t<Cs...> foreign_key(Cs... columns) {
        return {{std::forward<Cs>(columns)...}};
    }

    /**
     *  FOREIGN KEY constraint builder function taking one or more fields that are member pointers of base classes,
     *  specifying the derived class as an explicit template argument.
     *  Available since SQLite 3.6.19
     */
    template<class O, class... Base, class... F>
    constexpr internal::foreign_key_intermediate_t<F O::*...> foreign_key(F Base::*... columns) {
        static_assert(polyfill::conjunction<internal::is_field_of<F Base::*, O>...>::value,
                      "Fields must be from explicitly specified derived class");
        return {{columns...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  FOREIGN KEY constraint builder function taking one or more fields that are member pointers of base classes,
     *  specifying the derived class as an explicit template argument.
     *  Available since SQLite 3.6.19
     */
    template<orm_table_reference auto table, class... Base, class... F>
    constexpr auto foreign_key(F Base::*... columns) {
        return foreign_key<internal::auto_decay_table_ref_t<table>>(columns...);
    }
#endif
#endif

    /**
     *  UNIQUE table constraint builder function.
     */
    template<class... Args>
    constexpr internal::unique_t<Args...> unique(Args... args) {
        return {{std::forward<Args>(args)...}};
    }

    /**
     *  UNIQUE column constraint builder function.
     */
    inline constexpr internal::unique_t<> unique() {
        return {{}};
    }

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
    /**
     *  UNINDEXED column constraint builder function. Used in FTS virtual tables.
     *  
     *  https://www.sqlite.org/fts5.html#the_unindexed_column_option
     */
    inline constexpr internal::unindexed_t unindexed() {
        return {};
    }

    /**
     *  prefix=N table constraint builder function. Used in FTS virtual tables.
     *  
     *  https://www.sqlite.org/fts5.html#prefix_indexes
     */
    template<class T>
    constexpr internal::prefix_t<T> prefix(T value) {
        return {std::move(value)};
    }

    /**
     *  tokenize='...'' table constraint builder function. Used in FTS virtual tables.
     *  
     *  https://www.sqlite.org/fts5.html#tokenizers
     */
    template<class T>
    constexpr internal::tokenize_t<T> tokenize(T value) {
        return {std::move(value)};
    }

    /**
     *  content='' table constraint builder function. Used in FTS virtual tables.
     *  
     *  https://www.sqlite.org/fts5.html#contentless_tables
     */
    template<class T>
    constexpr internal::content_t<T> content(T value) {
        return {std::move(value)};
    }

    /**
     *  content='table' table constraint builder function. Used in FTS virtual tables.
     *  
     *  https://www.sqlite.org/fts5.html#external_content_tables
     */
    template<class T>
    constexpr internal::table_content_t<T> content() {
        return {};
    }

#if SQLITE_VERSION_NUMBER >= 3024000
    /** 
     *  Auxiliary virtual table column
     */
    constexpr inline internal::auxiliary_t auxiliary() {
        return {};
    }
#endif

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  content='table' table constraint builder function. Used in FTS virtual tables.
     *  
     *  https://www.sqlite.org/fts5.html#external_content_tables
     */
    template<orm_table_reference auto table>
    constexpr auto content() {
        return content<internal::auto_decay_table_ref_t<table>>();
    }
#endif
#endif

    /**
     *  PRIMARY KEY constraint builder function taking one or more fields, which can either be pointers to class members or column pointers.
     */
    template<class... Cs>
    constexpr internal::primary_key_t<Cs...> primary_key(Cs... cs) {
        return {{std::forward<Cs>(cs)...}};
    }

    /**
     *  PRIMARY KEY constraint builder function taking one or more fields that are member pointers of base classes,
     *  specifying the derived class as an explicit template argument.
     */
    template<class O, class... Base, class... F>
    constexpr internal::primary_key_t<F O::*...> primary_key(F Base::*... columns) {
        static_assert(polyfill::conjunction<internal::is_field_of<F Base::*, O>...>::value,
                      "Fields must be from explicitly specified derived class");
        return {{columns...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  PRIMARY KEY constraint builder function taking one or more fields that are member pointers of base classes,
     *  specifying the derived class as an explicit template argument.
     */
    template<orm_table_reference auto table, class... Base, class... F>
    constexpr auto primary_key(F Base::*... columns) {
        return primary_key<internal::auto_decay_table_ref_t<table>>(columns...);
    }
#endif

    /**
     *  PRIMARY KEY column constraint builder function (used at a single column).
     */
    inline constexpr internal::primary_key_t<> primary_key() {
        return {{}};
    }

    template<class T>
    constexpr internal::default_t<T> default_value(T t) {
        return {std::move(t)};
    }

    inline constexpr internal::collate_constraint_t collate_nocase() {
        return {internal::collate_argument::nocase};
    }

    inline constexpr internal::collate_constraint_t collate_binary() {
        return {internal::collate_argument::binary};
    }

    inline constexpr internal::collate_constraint_t collate_rtrim() {
        return {internal::collate_argument::rtrim};
    }

    template<class T>
    constexpr internal::check_t<T> check(T t) {
        return {std::move(t)};
    }

    inline constexpr internal::null_t null() {
        return {};
    }

    inline constexpr internal::not_null_t not_null() {
        return {};
    }
}
