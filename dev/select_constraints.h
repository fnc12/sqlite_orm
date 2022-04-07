#pragma once

#include <string>  //  std::string
#include <utility>  //  std::declval
#include <tuple>  //  std::tuple, std::get, std::tuple_size
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

#include "cxx_polyfill.h"
#include "type_traits.h"
#include "is_base_of_template.h"
#include "tuple_helper/tuple_helper.h"
#include "optional_container.h"
#include "ast/where.h"
#include "ast/group_by.h"
#include "core_functions.h"

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
        };

        template<class T>
        struct is_columns : std::false_type {};

        template<class... Args>
        struct is_columns<columns_t<Args...>> : std::true_type {};

        template<class... Args>
        struct set_t {
            using assigns_type = std::tuple<Args...>;

            assigns_type assigns;
        };

        /**
         *  This class is used to store explicit mapped type T and its column descriptor (member pointer/getter/setter).
         *  Is useful when mapped type is derived from other type and base class has members mapped to a storage.
         */
        template<class T, class F>
        struct column_pointer {
            using self = column_pointer<T, F>;
            using type = T;
            using field_type = F;

            field_type field;

            template<class R>
            internal::is_equal_t<self, R> operator==(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            internal::is_not_equal_t<self, R> operator!=(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            internal::lesser_than_t<self, R> operator<(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            internal::lesser_or_equal_t<self, R> operator<=(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            internal::greater_than_t<self, R> operator>(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            internal::greater_or_equal_t<self, R> operator>=(R rhs) const {
                return {*this, std::move(rhs)};
            }
        };

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
        };

        template<class T>
        struct is_select : std::false_type {};

        template<class T, class... Args>
        struct is_select<select_t<T, Args...>> : std::true_type {};

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

        struct union_base {
            bool all = false;

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

            union_t(left_type l, right_type r, decltype(all) all_) :
                compound_operator<L, R>(std::move(l), std::move(r)), union_base{all_} {}

            union_t(left_type l, right_type r) : union_t(std::move(l), std::move(r), false) {}
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

        struct with_string {
            explicit operator std::string() const {
                return "WITH";
            }
        };

        /**
         *  Labeled (aliased) CTE expression.
         */
        template<class Label, class Select, class nExplicitCols>
        struct common_table_expression : public Label {
            using cte_label_type = Label;
            using expression_type = Select;
            static constexpr size_t explicit_column_count = nExplicitCols::value;

            std::vector<std::string> explicitColumnNames;
            expression_type subselect;

            common_table_expression(std::vector<std::string> explicitColumnNames, expression_type subselect) :
                explicitColumnNames{move(explicitColumnNames)}, subselect{std::move(subselect)} {
                this->subselect.highest_level = true;
            }
        };
        template<class... CTEs>
        using common_table_expressions = std::tuple<CTEs...>;

        template<typename Label, class nExplicitCols>
        struct cte_builder {
            std::vector<std::string> explicitColumnNames;

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, nExplicitCols>
            operator()(select_t<T, Args...> sel) && {
                return {move(this->explicitColumnNames), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, nExplicitCols> operator()(Compound sel) && {
                return {move(this->explicitColumnNames), {std::move(sel)}};
            }

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, nExplicitCols> as(select_t<T, Args...> sel) && {
                return {move(this->explicitColumnNames), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, nExplicitCols> as(Compound sel) && {
                return {move(this->explicitColumnNames), {std::move(sel)}};
            }

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, nExplicitCols>
            materialized(select_t<T, Args...> sel) && {
                static_assert(polyfill::always_false_v<T>, "`WITH ... AS MATERIALIZED` is unimplemented");
                return {move(this->explicitColumnNames), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, nExplicitCols> materialized(Compound sel) && {
                static_assert(polyfill::always_false_v<Compound>, "`WITH ... AS MATERIALIZED` is unimplemented");
                return {move(this->explicitColumnNames), {std::move(sel)}};
            }

            template<class T, class... Args>
            common_table_expression<Label, select_t<T, Args...>, nExplicitCols>
            not_materialized(select_t<T, Args...> sel) && {
                static_assert(polyfill::always_false_v<T>, "`WITH ... AS NOT MATERIALIZED` is unimplemented");
                return {move(this->explicitColumnNames), std::move(sel)};
            }

            template<class Compound,
                     std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
            common_table_expression<Label, select_t<Compound>, nExplicitCols> not_materialized(Compound sel) && {
                static_assert(polyfill::always_false_v<Compound>, "`WITH ... AS NOT MATERIALIZED` is unimplemented");
                return {move(this->explicitColumnNames), {std::move(sel)}};
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

            with_t(cte_type cte, expression_type expression) : cte{move(cte)}, expression{std::move(expression)} {
                this->expression.highest_level = true;
            }
        };

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
        };

        template<class T>
        struct object_t {
            using type = T;
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
                result_args_type result_args =
                    std::tuple_cat(std::move(this->args), std::move(std::make_tuple(newPair)));
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

    /**
     *  SET keyword used in UPDATE ... SET queries.
     *  Args must have `assign_t` type. E.g. set(assign(&User::id, 5)) or set(c(&User::id) = 5)
     */
    template<class... Args>
    internal::set_t<Args...> set(Args... args) {
        using arg_tuple = std::tuple<Args...>;
        static_assert(std::tuple_size<arg_tuple>::value ==
                          internal::count_tuple<arg_tuple, internal::is_assign_t>::value,
                      "set function accepts assign operators only");
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    template<class... Args>
    internal::columns_t<Args...> columns(Args... args) {
        return {std::make_tuple<Args...>(std::forward<Args>(args)...)};
    }

    /**
     *  Explicitly refer to a column, used in contexts
     *  where the automatic object mapping deduction needs to be overridden,
     *  or when you need to refer to a column in a CTE.
     *  
     *  Use it like this:
     *  struct MyType : BaseType { ... };
     *  storage.select(column<MyType>(&BaseType::id));
     *  // ... or
     *  struct Object { ... };
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(0_col)));
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(c_v<&Object::id>)));
     */
    template<class O, class F>
    internal::column_pointer<O, F> column(F f) {
        return {std::move(f)};
    }

    /**
     *  Turn 1_nth_col -> 0_col
     */
    template<class O, unsigned int I>
    auto column(internal::nth_constant<I>) {
        return column<O>(polyfill::index_constant<I - 1>{});
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

    /**
     *  Example : cte<cte_1>()(select(&Object::id));
     *  The list of column names is optional;
     *  if provided the number of columns must match the number of columns of the subselect.
     *  The column names will be merged with the subselect:
     *  1. column names of subselect
     *  2. explicit columns
     *  3. fill in empty column names with stringified column index
     */
    template<class Label,
             class... ColumnNames,
             std::enable_if_t<polyfill::conjunction_v<std::is_convertible<ColumnNames, std::string>...>, bool> = true>
    internal::cte_builder<Label, polyfill::index_constant<sizeof...(ColumnNames)>>
    cte(ColumnNames... explicitColumnNames) {
        return {{std::move(explicitColumnNames)...}};
    }

    // tuple of CTEs
    template<class E, class... Labels, class... Selects, class... nExplicitCols>
    internal::with_t<E, internal::common_table_expression<Labels, Selects, nExplicitCols>...>
    with(std::tuple<internal::common_table_expression<Labels, Selects, nExplicitCols>...> cte, E expression) {
        return {move(cte), std::move(expression)};
    }

    /** A single CTE.
     *  Example : with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(0_col)));
     */
    template<class E, class Label, class Select, class nExplicitCols>
    internal::with_t<E, internal::common_table_expression<Label, Select, nExplicitCols>>
    with(internal::common_table_expression<Label, Select, nExplicitCols> cte, E expression) {
        return {std::make_tuple(move(cte)), std::move(expression)};
    }

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
     * SELECT * FROM T function.
     * T is typed mapped to a storage.
     * Example: auto rows = storage.select(asterisk<User>());
     * // decltype(rows) is std::vector<std::tuple<...all column typed in declared in make_table order...>>
     * If you need to fetch result as objects not tuple please use `object<T>` instead.
     */
    template<class T>
    internal::asterisk_t<T> asterisk() {
        return {};
    }

    /**
     * SELECT * FROM T function.
     * T is typed mapped to a storage.
     * Example: auto rows = storage.select(object<User>());
     * // decltype(rows) is std::vector<User>
     * If you need to fetch result as tuples not objects please use `asterisk<T>` instead.
     */
    template<class T>
    internal::object_t<T> object() {
        return {};
    }
}
