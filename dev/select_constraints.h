#pragma once

#include <type_traits>  //  std::remove_const
#include <string>  //  std::string
#include <utility>  //  std::move
#include <tuple>  //  std::tuple, std::get, std::tuple_size
#include "functional/cxx_optional.h"

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "is_base_of_template.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"
#include "optional_container.h"
#include "ast/where.h"
#include "ast/group_by.h"
#include "core_functions.h"
#include "alias_traits.h"
#include "column_pointer.h"

namespace sqlite_orm {

    namespace internal {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct as_optional_t {
            using value_type = T;

            value_type value;
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        struct distinct_string {
            operator std::string() const {
                return "DISTINCT";
            }
        };

        /**
         *  DISCTINCT generic container.
         */
        template<class T>
        struct distinct_t : distinct_string {
            using value_type = T;

            value_type value;

            distinct_t(value_type value_) : value(std::move(value_)) {}
        };

        struct all_string {
            operator std::string() const {
                return "ALL";
            }
        };

        /**
         *  ALL generic container.
         */
        template<class T>
        struct all_t : all_string {
            T value;

            all_t(T value_) : value(std::move(value_)) {}
        };

        template<class... Args>
        struct columns_t {
            using columns_type = std::tuple<Args...>;

            columns_type columns;
            bool distinct = false;

            static constexpr int count = std::tuple_size<columns_type>::value;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            columns_t(columns_type columns) : columns{std::move(columns)} {}
#endif
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_columns_v = polyfill::is_specialization_of_v<T, columns_t>;

        template<class T>
        using is_columns = polyfill::bool_constant<is_columns_v<T>>;

        /**
         *  Subselect object type.
         */
        template<class T, class... Args>
        struct select_t {
            using return_type = T;
            using conditions_type = std::tuple<Args...>;

            return_type col;
            conditions_type conditions;
            bool highest_level = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            select_t(return_type col, conditions_type conditions) :
                col{std::move(col)}, conditions{std::move(conditions)} {}
#endif
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_select_v = polyfill::is_specialization_of_v<T, select_t>;

        template<class T>
        using is_select = polyfill::bool_constant<is_select_v<T>>;

        /**
         *  Base for UNION, UNION ALL, EXCEPT and INTERSECT
         */
        template<class L, class R>
        struct compound_operator {
            using left_type = L;
            using right_type = R;

            left_type left;
            right_type right;

            compound_operator(left_type l, right_type r) : left(std::move(l)), right(std::move(r)) {
                this->left.highest_level = true;
                this->right.highest_level = true;
            }
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_compound_operator_v = is_base_of_template_v<T, compound_operator>;

        template<class T>
        using is_compound_operator = polyfill::bool_constant<is_compound_operator_v<T>>;

        struct union_base {
            bool all = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            union_base(bool all) : all{all} {}
#endif

            operator std::string() const {
                if(!this->all) {
                    return "UNION";
                } else {
                    return "UNION ALL";
                }
            }
        };

        /**
         *  UNION object type.
         */
        template<class L, class R>
        struct union_t : public compound_operator<L, R>, union_base {
            using left_type = typename compound_operator<L, R>::left_type;
            using right_type = typename compound_operator<L, R>::right_type;

            union_t(left_type l, right_type r, bool all_) :
                compound_operator<L, R>{std::move(l), std::move(r)}, union_base{all_} {}

            union_t(left_type l, right_type r) : union_t{std::move(l), std::move(r), false} {}
        };

        struct except_string {
            operator std::string() const {
                return "EXCEPT";
            }
        };

        /**
         *  EXCEPT object type.
         */
        template<class L, class R>
        struct except_t : compound_operator<L, R>, except_string {
            using super = compound_operator<L, R>;
            using left_type = typename super::left_type;
            using right_type = typename super::right_type;

            using super::super;
        };

        struct intersect_string {
            operator std::string() const {
                return "INTERSECT";
            }
        };
        /**
         *  INTERSECT object type.
         */
        template<class L, class R>
        struct intersect_t : compound_operator<L, R>, intersect_string {
            using super = compound_operator<L, R>;
            using left_type = typename super::left_type;
            using right_type = typename super::right_type;

            using super::super;
        };

#ifdef SQLITE_ORM_WITH_CTE
        struct with_string {
            explicit operator std::string() const {
                return "WITH";
            }
        };

        /*
         *  Turn explicit columns for a CTE into types that the CTE backend understands
         */
        template<class T, class SFINAE = void>
        struct decay_explicit_column {
            using type = T;
        };
        template<class T>
        struct decay_explicit_column<T, match_if<is_column_alias, T>> {
            using type = alias_holder<T>;
        };
        template<class T>
        struct decay_explicit_column<T, match_if<std::is_convertible, T, std::string>> {
            using type = std::string;
        };
        template<class T>
        using decay_explicit_column_t = typename decay_explicit_column<T>::type;

        /**
         *  Labeled (aliased) CTE expression.
         */
        template<class Label, class Select, class ExplicitCols>
        struct common_table_expression {
            using cte_label_type = Label;
            using expression_type = Select;
            using explicit_colrefs_tuple = ExplicitCols;
            static constexpr size_t explicit_colref_count = std::tuple_size_v<ExplicitCols>;

            explicit_colrefs_tuple explicitColumns;
            expression_type subselect;

            common_table_expression(explicit_colrefs_tuple explicitColumns, expression_type subselect) :
                explicitColumns{std::move(explicitColumns)}, subselect{std::move(subselect)} {
                this->subselect.highest_level = true;
            }
        };
        template<class... CTEs>
        using common_table_expressions = std::tuple<CTEs...>;

        template<typename Label, class ExplicitCols>
        struct cte_builder {
            ExplicitCols explicitColumns;

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, ExplicitCols> operator()(select_t<T, Args...> sel) && {
                return {std::move(this->explicitColumns), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, ExplicitCols> operator()(Compound sel) && {
                return {std::move(this->explicitColumns), {std::move(sel)}};
            }

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, ExplicitCols> as(select_t<T, Args...> sel) && {
                return {std::move(this->explicitColumns), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, ExplicitCols> as(Compound sel) && {
                return {std::move(this->explicitColumns), {std::move(sel)}};
            }

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, ExplicitCols>
            materialized(select_t<T, Args...> sel) && {
                static_assert(polyfill::always_false_v<T>, "`WITH ... AS MATERIALIZED` is unimplemented");
                return {std::move(this->explicitColumns), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, ExplicitCols> materialized(Compound sel) && {
                static_assert(polyfill::always_false_v<Compound>, "`WITH ... AS MATERIALIZED` is unimplemented");
                return {std::move(this->explicitColumns), {std::move(sel)}};
            }

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, ExplicitCols>
            not_materialized(select_t<T, Args...> sel) && {
                static_assert(polyfill::always_false_v<T>, "`WITH ... AS NOT MATERIALIZED` is unimplemented");
                return {std::move(this->explicitColumns), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, ExplicitCols> not_materialized(Compound sel) && {
                static_assert(polyfill::always_false_v<Compound>, "`WITH ... AS NOT MATERIALIZED` is unimplemented");
                return {std::move(this->explicitColumns), {std::move(sel)}};
            }
        };

        /**
         *  WITH object type - expression with prepended CTEs.
         */
        template<class E, class... CTEs>
        struct with_t : with_string {
            using cte_type = common_table_expressions<CTEs...>;
            using expression_type = E;

            cte_type cte;
            expression_type expression;

            with_t(cte_type cte, expression_type expression) : cte{std::move(cte)}, expression{std::move(expression)} {
                this->expression.highest_level = true;
            }
        };
#endif

        /**
         *  Generic way to get DISTINCT value from any type.
         */
        template<class T>
        bool get_distinct(const T&) {
            return false;
        }

        template<class... Args>
        bool get_distinct(const columns_t<Args...>& cols) {
            return cols.distinct;
        }

        template<class T>
        struct asterisk_t {
            using type = T;

            bool defined_order = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            asterisk_t(bool definedOrder) : defined_order{definedOrder} {}
#endif
        };

        template<class T>
        struct object_t {
            using type = T;

            bool defined_order = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            object_t(bool definedOrder) : defined_order{definedOrder} {}
#endif
        };

        template<class T>
        struct then_t {
            using expression_type = T;

            expression_type expression;
        };

        template<class R, class T, class E, class... Args>
        struct simple_case_t {
            using return_type = R;
            using case_expression_type = T;
            using args_type = std::tuple<Args...>;
            using else_expression_type = E;

            optional_container<case_expression_type> case_expression;
            args_type args;
            optional_container<else_expression_type> else_expression;
        };

        /**
         *  T is a case expression type
         *  E is else type (void is ELSE is omitted)
         *  Args... is a pack of WHEN expressions
         */
        template<class R, class T, class E, class... Args>
        struct simple_case_builder {
            using return_type = R;
            using case_expression_type = T;
            using args_type = std::tuple<Args...>;
            using else_expression_type = E;

            optional_container<case_expression_type> case_expression;
            args_type args;
            optional_container<else_expression_type> else_expression;

            template<class W, class Th>
            simple_case_builder<R, T, E, Args..., std::pair<W, Th>> when(W w, then_t<Th> t) {
                using result_args_type = std::tuple<Args..., std::pair<W, Th>>;
                std::pair<W, Th> newPair{std::move(w), std::move(t.expression)};
                result_args_type result_args = std::tuple_cat(std::move(this->args), std::make_tuple(newPair));
                std::get<std::tuple_size<result_args_type>::value - 1>(result_args) = std::move(newPair);
                return {std::move(this->case_expression), std::move(result_args), std::move(this->else_expression)};
            }

            simple_case_t<R, T, E, Args...> end() {
                return {std::move(this->case_expression), std::move(args), std::move(this->else_expression)};
            }

            template<class El>
            simple_case_builder<R, T, El, Args...> else_(El el) {
                return {{std::move(this->case_expression)}, std::move(args), {std::move(el)}};
            }
        };

        template<class T>
        void validate_conditions() {
            static_assert(count_tuple<T, is_where>::value <= 1, "a single query cannot contain > 1 WHERE blocks");
            static_assert(count_tuple<T, is_group_by>::value <= 1, "a single query cannot contain > 1 GROUP BY blocks");
            static_assert(count_tuple<T, is_order_by>::value <= 1, "a single query cannot contain > 1 ORDER BY blocks");
            static_assert(count_tuple<T, is_limit>::value <= 1, "a single query cannot contain > 1 LIMIT blocks");
            static_assert(count_tuple<T, is_from>::value <= 1, "a single query cannot contain > 1 FROM blocks");
        }
    }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    internal::as_optional_t<T> as_optional(T value) {
        return {std::move(value)};
    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    internal::then_t<T> then(T t) {
        return {std::move(t)};
    }

    template<class R, class T>
    internal::simple_case_builder<R, T, void> case_(T t) {
        return {{std::move(t)}};
    }

    template<class R>
    internal::simple_case_builder<R, void, void> case_() {
        return {};
    }

    template<class T>
    internal::distinct_t<T> distinct(T t) {
        return {std::move(t)};
    }

    template<class T>
    internal::all_t<T> all(T t) {
        return {std::move(t)};
    }

    template<class... Args>
    internal::columns_t<Args...> distinct(internal::columns_t<Args...> cols) {
        cols.distinct = true;
        return cols;
    }

    template<class... Args>
    internal::columns_t<Args...> columns(Args... args) {
        return {std::make_tuple<Args...>(std::forward<Args>(args)...)};
    }

    /**
     *  Public function for subselect query. Is useful in UNION queries.
     */
    template<class T, class... Args>
    internal::select_t<T, Args...> select(T t, Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        return {std::move(t), std::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  Public function for UNION operator.
     *  lhs and rhs are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class L, class R>
    internal::union_t<L, R> union_(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs)};
    }

    /**
     *  Public function for EXCEPT operator.
     *  lhs and rhs are subselect objects.
     *  Look through example in examples/except.cpp
     */
    template<class L, class R>
    internal::except_t<L, R> except(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs)};
    }

    template<class L, class R>
    internal::intersect_t<L, R> intersect(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs)};
    }

#ifdef SQLITE_ORM_WITH_CTE
    /**
     *  Example : cte<cte_1>()(select(&Object::id));
     *  The list of explicit columns is optional;
     *  if provided the number of columns must match the number of columns of the subselect.
     *  The column names will be merged with the subselect:
     *  1. column names of subselect
     *  2. explicit columns
     *  3. fill in empty column names with column index
     */
    template<class Label,
             class... ExplicitCols,
             std::enable_if_t<polyfill::conjunction_v<polyfill::disjunction<
                                  internal::is_column_alias<ExplicitCols>,
                                  std::is_member_pointer<ExplicitCols>,
                                  internal::is_column<ExplicitCols>,
                                  std::is_same<ExplicitCols, polyfill::remove_cvref_t<decltype(std::ignore)>>,
                                  std::is_convertible<ExplicitCols, std::string>>...>,
                              bool> = true>
    auto cte(ExplicitCols... explicitColumns) {
        static_assert(internal::is_cte_alias_v<Label>, "Label must be a CTE alias");
#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
        static_assert((!internal::is_builtin_numeric_column_alias_v<ExplicitCols> && ...),
                      "Numeric column aliases are reserved for referencing columns locally within a single CTE.");
#endif

        using builder_type = internal::cte_builder<
            Label,
            internal::transform_tuple_t<std::tuple<ExplicitCols...>, internal::decay_explicit_column_t>>;
        return builder_type{{std::move(explicitColumns)...}};
    }

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
    template<auto label,
             class... ExplicitCols,
             std::enable_if_t<polyfill::conjunction_v<polyfill::disjunction<
                                  internal::is_column_alias<ExplicitCols>,
                                  std::is_member_pointer<ExplicitCols>,
                                  internal::is_column<ExplicitCols>,
                                  std::is_same<ExplicitCols, std::remove_cvref_t<decltype(std::ignore)>>,
                                  std::is_convertible<ExplicitCols, std::string>>...>,
                              bool> = true>
    auto cte(ExplicitCols... explicitColumns) {
        static_assert(internal::is_cte_alias_v<decltype(label)>, "Label must be a CTE alias");
        static_assert((!internal::is_builtin_numeric_column_alias_v<ExplicitCols> && ...),
                      "Numeric column aliases are reserved for referencing columns locally within a single CTE.");

        using builder_type = internal::cte_builder<
            decltype(label),
            internal::transform_tuple_t<std::tuple<ExplicitCols...>, internal::decay_explicit_column_t>>;
        return builder_type{{std::move(explicitColumns)...}};
    }
#endif

    // tuple of CTEs
    template<class E, class... Labels, class... Selects, class... ExplicitCols>
    internal::with_t<E, internal::common_table_expression<Labels, Selects, ExplicitCols>...>
    with(std::tuple<internal::common_table_expression<Labels, Selects, ExplicitCols>...> cte, E expression) {
        return {std::move(cte), std::move(expression)};
    }

    /** A single CTE.
     *  Example : with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(0_col)));
     */
    template<class E, class Label, class Select, class ExplicitCols>
    internal::with_t<E, internal::common_table_expression<Label, Select, ExplicitCols>>
    with(internal::common_table_expression<Label, Select, ExplicitCols> cte, E expression) {
        return {std::make_tuple(std::move(cte)), std::move(expression)};
    }
#endif

    /**
     *  Public function for UNION ALL operator.
     *  lhs and rhs are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class L, class R>
    internal::union_t<L, R> union_all(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs), true};
    }

    /**
     *   `SELECT * FROM T` expression that fetches results as tuples.
     *   T is a type mapped to a storage, or an alias of it.
     *   The `definedOrder` parameter denotes the expected order of result columns.
     *   The default is the implicit order as returned by SQLite, which may differ from the defined order
     *   if the schema of a table has been changed.
     *   By specifying the defined order, the columns are written out in the resulting select SQL string.
     *
     *   In pseudo code:
     *   select(asterisk<User>(false)) -> SELECT * from User
     *   select(asterisk<User>(true))  -> SELECT id, name from User
     *
     *   Example: auto rows = storage.select(asterisk<User>());
     *   // decltype(rows) is std::vector<std::tuple<...all columns in implicitly stored order...>>
     *   Example: auto rows = storage.select(asterisk<User>(true));
     *   // decltype(rows) is std::vector<std::tuple<...all columns in declared make_table order...>>
     *   
     *   If you need to fetch results as objects instead of tuples please use `object<T>()`.
     */
    template<class T>
    internal::asterisk_t<T> asterisk(bool definedOrder = false) {
        return {definedOrder};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_recordset_alias auto als>
    auto asterisk(bool definedOrder = false) {
        return internal::asterisk_t<std::remove_const_t<decltype(als)>>{definedOrder};
    }
#endif

    /**
     *   `SELECT * FROM T` expression that fetches results as objects of type T.
     *   T is a type mapped to a storage, or an alias of it.
     *   
     *   Example: auto rows = storage.select(object<User>());
     *   // decltype(rows) is std::vector<User>, where the User objects are constructed from columns in implicitly stored order
     *   Example: auto rows = storage.select(object<User>(true));
     *   // decltype(rows) is std::vector<User>, where the User objects are constructed from columns in declared make_table order
     *
     *   If you need to fetch results as tuples instead of objects please use `asterisk<T>()`.
     */
    template<class T>
    internal::object_t<T> object(bool definedOrder = false) {
        return {definedOrder};
    }
}
