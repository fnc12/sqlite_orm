#pragma once

/** @file Binding the parameters of a statement in sequence, value by value through `statement_binder`:
 *        the bindable leaves of an expression, the fields of an object, the elements of a tuple.
 */

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::make_index_sequence, std::index_sequence
#include <tuple>  //  std::tuple_size, std::get
#include <utility>  //  std::forward
#include <functional>  //  std::invoke
#include <system_error>  //  std::system_error
#endif

#include "../functional/type_traits.h"
#include "../tuple_helper/tuple_filter.h"
#include "../vocabulary/node_algorithms.h"  //  is_bindable
#include "../error_code.h"
#include "../sqlite3/sqlite3_errors.h"
#include "statement_binder.h"

namespace sqlite_orm::internal {
    struct conditional_binder {
        sqlite3_stmt* stmt = nullptr;
        int nthSqlParameter = 0;

        explicit conditional_binder(sqlite3_stmt* stmt) : stmt{stmt} {}

        template<class T, satisfies<is_bindable, T> = true>
        void operator()(const T& t) {
            const int rc = statement_binder<T>{}.bind(this->stmt, ++this->nthSqlParameter, t);
            if (SQLITE_OK != rc) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                throw_translated_sqlite_error(rc);
            }
        }

        template<class T, satisfies_not<is_bindable, T> = true>
        void operator()(const T&) const {}
    };

    struct field_value_binder : conditional_binder {
        using conditional_binder::conditional_binder;
        using conditional_binder::operator();

        template<class T, satisfies_not<is_bindable, T> = true>
        void operator()(const T&) const = delete;

        template<class T>
        void operator()(const T* value) {
            if (!value) {
                throw std::system_error{orm_error_code::value_is_null};
            }
            (*this)(*value);
        }
    };

    struct tuple_value_binder {
        sqlite3_stmt* stmt = nullptr;

        explicit tuple_value_binder(sqlite3_stmt* stmt) : stmt{stmt} {}

#ifdef SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
        template<class Tpl, class Projection>
        void operator()(const Tpl& tpl, Projection project) const {
            int nthSqlParameter = 0;
            auto& [... elements] = tpl;
            (this->bind(std::invoke(project, elements), ++nthSqlParameter), ...);
        }
#else
        template<class Tpl, class Projection>
        void operator()(const Tpl& tpl, Projection project) const {
            (*this)(tpl, std::make_index_sequence<std::tuple_size<Tpl>::value>{}, std::forward<Projection>(project));
        }
#endif

      private:
#ifndef SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
        template<class Tpl, size_t... Idx, class Projection>
        void operator()(const Tpl& tpl, std::index_sequence<Idx...>, Projection project) const {
            (this->bind(std::invoke(project, std::get<Idx>(tpl)), int(Idx + 1)), ...);
        }
#endif

        template<class T>
        void bind(const T& t, int nthSqlParameter) const {
            const int rc = statement_binder<T>{}.bind(this->stmt, nthSqlParameter, t);
            if (SQLITE_OK != rc) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                throw_translated_sqlite_error(rc);
            }
        }

        template<class T>
        void bind(const T* value, int nthSqlParameter) const {
            if (!value) {
                throw std::system_error{orm_error_code::value_is_null};
            }
            this->bind(*value, nthSqlParameter);
        }
    };

    template<class Tpl>
    using bindable_filter_t = filter_tuple_t<Tpl, is_bindable>;
}
