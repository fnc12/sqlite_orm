#pragma once

/** @file The joins of a FROM clause, with their ON and USING constraints.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/mpl.h"
#include "../functional/type_traits.h"
#include "../table_reference.h"
#include "../column_pointer.h"
#include "../vocabulary/node_traits.h"  //  on_type_t
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "cross_join.h"

namespace sqlite_orm::internal {
    /**
     *  NATURAL JOIN holder.
     *  T is joined type which represents any mapped table.
     */
    template<class T>
    struct natural_join_t {
        using type = T;
    };

    struct left_join_string {
        operator std::string() const {
            return "LEFT JOIN";
        }
    };

    /**
     *  LEFT JOIN holder.
     *  T is joined type which represents any mapped table.
     *  O is on(...) argument type.
     */
    template<class T, class O>
    struct left_join_t : left_join_string {
        using type = T;
        using on_type = O;

        on_type constraint;

        left_join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
    };

    struct join_string {
        operator std::string() const {
            return "JOIN";
        }
    };

    /**
     *  Simple JOIN holder.
     *  T is joined type which represents any mapped table.
     *  O is on(...) argument type.
     */
    template<class T, class O>
    struct join_t : join_string {
        using type = T;
        using on_type = O;

        on_type constraint;

        join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
    };

    struct left_outer_join_string {
        operator std::string() const {
            return "LEFT OUTER JOIN";
        }
    };

    /**
     *  LEFT OUTER JOIN holder.
     *  T is joined type which represents any mapped table.
     *  O is on(...) argument type.
     */
    template<class T, class O>
    struct left_outer_join_t : left_outer_join_string {
        using type = T;
        using on_type = O;

        on_type constraint;

        left_outer_join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
    };

    struct on_string {
        operator std::string() const {
            return "ON";
        }
    };

    /**
     *  on(...) argument holder used for JOIN, LEFT JOIN, LEFT OUTER JOIN and INNER JOIN
     *  T is on type argument.
     */
    template<class T>
    struct on_t : on_string {
        using arg_type = T;

        arg_type arg;

        on_t(arg_type arg_) : arg(std::move(arg_)) {}
    };

    /**
     *  USING argument holder.
     */
    template<class T, class M>
    struct using_t {
        column_pointer<T, M> column;

        operator std::string() const {
            return "USING";
        }
    };

    struct inner_join_string {
        operator std::string() const {
            return "INNER JOIN";
        }
    };

    /**
     *  INNER JOIN holder.
     *  T is joined type which represents any mapped table.
     *  O is on(...) argument type.
     */
    template<class T, class O>
    struct inner_join_t : inner_join_string {
        using type = T;
        using on_type = O;

        on_type constraint;

        inner_join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
    };

    template<class T>
    using is_constrained_join = polyfill::is_detected<on_type_t, T>;

    template<class T>
    constexpr bool is_any_join_v = mpl::invoke_t<mpl::disjunction<check_if<is_constrained_join>,
                                                                  check_if_is_template<cross_join_t>,
                                                                  check_if_is_template<natural_join_t>>,
                                                 T>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class F, class O>
    internal::using_t<O, F O::*> using_(F O::* field) {
        return {field};
    }
    template<class T, class M>
    internal::using_t<T, M> using_(internal::column_pointer<T, M> field) {
        return {std::move(field)};
    }

    template<class T>
    internal::on_t<T> on(T t) {
        return {std::move(t)};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_recordset auto alias>
    auto cross_join() {
        return cross_join<internal::auto_decay_table_ref_t<alias>>();
    }
#endif

    template<class T>
    internal::natural_join_t<T> natural_join() {
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_recordset auto alias>
    auto natural_join() {
        return natural_join<internal::auto_decay_table_ref_t<alias>>();
    }
#endif

    template<class T, class O>
    internal::left_join_t<T, O> left_join(O o) {
        return {std::move(o)};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_recordset auto alias, class On>
    auto left_join(On on) {
        return left_join<internal::auto_decay_table_ref_t<alias>, On>(std::move(on));
    }
#endif

    template<class T, class O>
    internal::join_t<T, O> join(O o) {
        return {std::move(o)};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_recordset auto alias, class On>
    auto join(On on) {
        return join<internal::auto_decay_table_ref_t<alias>, On>(std::move(on));
    }
#endif

    template<class T, class O>
    internal::left_outer_join_t<T, O> left_outer_join(O o) {
        return {std::move(o)};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_recordset auto alias, class On>
    auto left_outer_join(On on) {
        return left_outer_join<internal::auto_decay_table_ref_t<alias>, On>(std::move(on));
    }
#endif

    template<class T, class O>
    internal::inner_join_t<T, O> inner_join(O o) {
        return {std::move(o)};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_recordset auto alias, class On>
    auto inner_join(On on) {
        return inner_join<internal::auto_decay_table_ref_t<alias>, On>(std::move(on));
    }
#endif
}
