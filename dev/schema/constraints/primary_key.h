#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of
#include <tuple>  //  std::tuple
#include <utility>  //  std::forward, std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../member_traits/field_of.h"
#include "../../alias_traits.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

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
        } _options;
    };

    template<class T>
    struct primary_key_with_autoincrement : T {
        using primary_key_type = T;

        const primary_key_type& as_base() const {
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

        columns_tuple _columns;

        constexpr primary_key_t(columns_tuple columns) : _columns(std::move(columns)) {}

        constexpr primary_key_t asc() const {
            auto res = *this;
            res._options.asc_option = order_by::ascending;
            return res;
        }

        constexpr primary_key_t desc() const {
            auto res = *this;
            res._options.asc_option = order_by::descending;
            return res;
        }

        constexpr primary_key_with_autoincrement<primary_key_t> autoincrement() const {
            return {*this};
        }

        constexpr primary_key_t on_conflict_rollback() const {
            auto res = *this;
            res._options.conflict_clause_is_on = true;
            res._options.conflict_clause = conflict_clause_t::rollback;
            return res;
        }

        constexpr primary_key_t on_conflict_abort() const {
            auto res = *this;
            res._options.conflict_clause_is_on = true;
            res._options.conflict_clause = conflict_clause_t::abort;
            return res;
        }

        constexpr primary_key_t on_conflict_fail() const {
            auto res = *this;
            res._options.conflict_clause_is_on = true;
            res._options.conflict_clause = conflict_clause_t::fail;
            return res;
        }

        constexpr primary_key_t on_conflict_ignore() const {
            auto res = *this;
            res._options.conflict_clause_is_on = true;
            res._options.conflict_clause = conflict_clause_t::ignore;
            return res;
        }

        constexpr primary_key_t on_conflict_replace() const {
            auto res = *this;
            res._options.conflict_clause_is_on = true;
            res._options.conflict_clause = conflict_clause_t::replace;
            return res;
        }
    };

    template<class T>
    constexpr bool is_primary_key_v = std::is_base_of<primary_key_base, T>::value;

    template<class T>
    constexpr bool is_column_primary_key_v = std::is_base_of<primary_key_t<>, T>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  PRIMARY KEY constraint factory function taking one or more fields, which can either be pointers to class members or column pointers.
     */
    template<class... Cs>
    constexpr internal::primary_key_t<Cs...> primary_key(Cs... cs) {
        return {{std::forward<Cs>(cs)...}};
    }

    /**
     *  PRIMARY KEY constraint factory function taking one or more fields that are member pointers of base classes,
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
     *  PRIMARY KEY constraint factory function taking one or more fields that are member pointers of base classes,
     *  specifying the derived class as an explicit template argument.
     */
    template<orm_table_reference auto table, class... Base, class... F>
    constexpr auto primary_key(F Base::*... columns) {
        return primary_key<internal::auto_decay_table_ref_t<table>>(columns...);
    }
#endif

    /**
     *  PRIMARY KEY column constraint factory function (used at a single column).
     */
    constexpr internal::primary_key_t<> primary_key() {
        return {{}};
    }
}
