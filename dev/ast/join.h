#pragma once

/** @file The joins of a FROM clause, with their ON and USING constraints.
 *
 *        All joins are DSL spellings of the one join-operator production, classified by `is_any_join`: each carries
 *        its keyword, the joined table as its `type`, and its constraint as its `on_type` - ON or USING spelled out,
 *        or the implicit constraint of CROSS JOIN and NATURAL JOIN.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::is_same
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

    /**
     *  A join of the table `T`, with the join operator `Keyword` and its constraint `Constraint`: ON or USING
     *  spelled out, or the implicit constraint of a join that spells none out.
     *
     *  Every join is this one node; the join operators are its aliases, telling them apart by their keyword.
     */
    template<class Keyword, class T, class Constraint>
    struct join_clause : Keyword {
        using type = T;
        using on_type = Constraint;

        SQLITE_ORM_NOUNIQUEADDRESS on_type constraint;

        join_clause() = default;

        join_clause(on_type constraint_) : constraint(std::move(constraint_)) {}
    };

    template<class T>
    constexpr bool is_any_join_v = polyfill::is_specialization_of<T, join_clause>::value;

    struct cross_join_string {
        operator std::string() const {
            return "CROSS JOIN";
        }
    };

    template<class T>
    using cross_join_t = join_clause<cross_join_string, T, implicit_join_constraint>;

    struct natural_join_string {
        operator std::string() const {
            return "NATURAL JOIN";
        }
    };

    template<class T>
    using natural_join_t = join_clause<natural_join_string, T, implicit_join_constraint>;

    struct join_string {
        operator std::string() const {
            return "JOIN";
        }
    };

    template<class T, class O>
    using join_t = join_clause<join_string, T, O>;

    struct inner_join_string {
        operator std::string() const {
            return "INNER JOIN";
        }
    };

    template<class T, class O>
    using inner_join_t = join_clause<inner_join_string, T, O>;

    struct left_join_string {
        operator std::string() const {
            return "LEFT JOIN";
        }
    };

    template<class T, class O>
    using left_join_t = join_clause<left_join_string, T, O>;

    struct left_outer_join_string {
        operator std::string() const {
            return "LEFT OUTER JOIN";
        }
    };

    template<class T, class O>
    using left_outer_join_t = join_clause<left_outer_join_string, T, O>;

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
