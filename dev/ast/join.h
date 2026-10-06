#pragma once

/** @file The joins of a FROM clause, with their ON and USING constraints.
 *
 *        All joins are DSL spellings of the one join-operator production, classified by `is_any_join`: each carries
 *        its keyword, the joined table as its `type`, and its constraint as its `on_type` - ON or USING spelled out,
 *        or the implicit constraint of CROSS JOIN and NATURAL JOIN.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::disjunction
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../table_reference.h"
#include "../column_pointer.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  The constraint of a join that spells none out: CROSS JOIN joins every row with every row, and NATURAL JOIN
     *  matches the columns both tables have in common. Every join is constrained - explicitly by ON or USING, or
     *  implicitly.
     */
    struct implicit_join_constraint {};

    template<class T>
    constexpr bool is_implicit_join_constraint_v = std::is_same<T, implicit_join_constraint>::value;

    struct cross_join_string {
        operator std::string() const {
            return "CROSS JOIN";
        }
    };

    /**
     *  CROSS JOIN holder.
     *  T is joined type which represents any mapped table.
     */
    template<class T>
    struct cross_join_t : cross_join_string {
        using type = T;
        using on_type = implicit_join_constraint;

        SQLITE_ORM_NOUNIQUEADDRESS on_type constraint;
    };

    struct natural_join_string {
        operator std::string() const {
            return "NATURAL JOIN";
        }
    };

    /**
     *  NATURAL JOIN holder.
     *  T is joined type which represents any mapped table.
     */
    template<class T>
    struct natural_join_t : natural_join_string {
        using type = T;
        using on_type = implicit_join_constraint;

        SQLITE_ORM_NOUNIQUEADDRESS on_type constraint;
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
    constexpr bool is_any_join_v = std::disjunction<polyfill::is_specialization_of<T, cross_join_t>,
                                                    polyfill::is_specialization_of<T, natural_join_t>,
                                                    polyfill::is_specialization_of<T, left_join_t>,
                                                    polyfill::is_specialization_of<T, join_t>,
                                                    polyfill::is_specialization_of<T, left_outer_join_t>,
                                                    polyfill::is_specialization_of<T, inner_join_t>>::value;

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
        using expression_type = T;

        expression_type arg;

        on_t(expression_type arg_) : arg(std::move(arg_)) {}
    };

    template<class T>
    constexpr bool is_on_v = polyfill::is_specialization_of_v<T, on_t>;

    /**
     *  USING argument holder.
     */
    template<class T, class M>
    struct using_t {
        using column_type = column_pointer<T, M>;

        column_type column;

        operator std::string() const {
            return "USING";
        }
    };

    template<class T>
    constexpr bool is_using_v = polyfill::is_specialization_of_v<T, using_t>;
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

    /**
     *  CROSS JOIN function. Usage:
     *  `cross_join<User>();`
     */
    template<class T>
    internal::cross_join_t<T> cross_join() {
        return {};
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
