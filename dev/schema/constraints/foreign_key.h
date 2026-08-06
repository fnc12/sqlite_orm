#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#include <ostream>  //  std::ostream
#include <string>  //  std::string
#include <tuple>  //  std::tuple, std::tuple_size
#include <functional>  //  std::ref
#include <utility>  //  std::forward, std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../tuple_helper/same_or_void.h"
#include "../../alias_traits.h"
#include "../../field_of.h"
#include "../../table_type_of.h"

namespace sqlite_orm::internal {
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

        on_update_delete_t(const on_update_delete_t&) = delete;
        on_update_delete_t& operator=(const on_update_delete_t&) = delete;

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
            on_update(std::ref(*this), true, foreign_key_action::none),
            on_delete(std::ref(*this), false, foreign_key_action::none) {}

        foreign_key_t(const foreign_key_t& other) :
            _columns(other._columns), _references(other._references),
            on_update(std::ref(*this), true, other.on_update._action),
            on_delete(std::ref(*this), false, other.on_delete._action) {}

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

    template<class T>
    constexpr bool is_foreign_key_v =
#if SQLITE_VERSION_NUMBER >= 3006019
        polyfill::is_specialization_of<T, foreign_key_t>::value;
#else
        false;
#endif

    template<class T>
    struct is_foreign_key : polyfill::bool_constant<is_foreign_key_v<T>> {};
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3006019
    /**
     *  FOREIGN KEY constraint factory function taking one or more fields, which can either be pointers to class members or column pointers.
     *  Available since SQLite 3.6.19
     */
    template<class... Cs>
    constexpr internal::foreign_key_intermediate_t<Cs...> foreign_key(Cs... columns) {
        return {{std::forward<Cs>(columns)...}};
    }

    /**
     *  FOREIGN KEY constraint factory function taking one or more fields that are member pointers of base classes,
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
     *  FOREIGN KEY constraint factory function taking one or more fields that are member pointers of base classes,
     *  specifying the derived class as an explicit template argument.
     *  Available since SQLite 3.6.19
     */
    template<orm_table_reference auto table, class... Base, class... F>
    constexpr auto foreign_key(F Base::*... columns) {
        return foreign_key<internal::auto_decay_table_ref_t<table>>(columns...);
    }
#endif
#endif
}
