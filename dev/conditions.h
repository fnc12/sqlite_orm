#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::enable_if, std::is_same, std::remove_const
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple
#include <utility>  //  std::move, std::forward
#include <sstream>  //  std::stringstream
#include <ostream>  //  std::flush
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/is_base_template_of.h"
#include "type_traits.h"
#include "collate_argument.h"
#include "constraints.h"
#include "optional_container.h"
#include "serializer_context.h"
#include "serialize_result_type.h"
#include "tags.h"
#include "table_reference.h"
#include "alias_traits.h"
#include "expression.h"
#include "column_pointer.h"
#include "type_printer.h"
#include "literal.h"
#include "ast/cross_join.h"
#include "ast/rank.h"

namespace sqlite_orm::internal {
    /**
     *  Collated something
     */
    template<class T>
    struct collate_t : condition_t {
        T expression;
        collate_argument argument;

        collate_t(T expression_, collate_argument argument_) :
            expression(std::move(expression_)), argument(argument_) {}
    };

    struct named_collate_base {
        std::string name;
    };

    /**
     *  Collated something with custom collate function
     */
    template<class T>
    struct named_collate : named_collate_base {
        T expression;

        named_collate(T expression_, std::string name_) :
            named_collate_base{std::move(name_)}, expression(std::move(expression_)) {}
    };

    struct negated_condition_string {
        operator std::string() const {
            return "NOT";
        }
    };

    /**
     *  Result of not operator
     */
    template<class C>
    struct negated_condition_t : condition_t, negated_condition_string {
        using argument_type = C;

        argument_type c;

        constexpr negated_condition_t(argument_type arg) : c(std::move(arg)) {}
    };

    /**
     *  Base class for binary conditions
     *  L is left argument type
     *  R is right argument type
     *  S is 'string' class (a class which has cast to `std::string` operator)
     *  Res is result type
     */
    template<class L, class R, class S, class Res>
    struct binary_condition : condition_t, S {
        using left_type = L;
        using right_type = R;
        using result_type = Res;

        left_type lhs;
        right_type rhs;

        constexpr binary_condition() = default;

        constexpr binary_condition(left_type l_, right_type r_) : lhs(std::move(l_)), rhs(std::move(r_)) {}
    };

    template<class T>
    inline constexpr bool is_binary_condition_v = is_base_template_of_v<binary_condition, T>;

    template<class T>
    struct is_binary_condition : polyfill::bool_constant<is_binary_condition_v<T>> {};

    struct and_condition_string {
        serialize_result_type serialize() const {
            return "AND";
        }
    };

    /**
     *  Result of and operator
     */
    template<class L, class R>
    struct and_condition_t : binary_condition<L, R, and_condition_string, bool>, negatable_t {
        using super = binary_condition<L, R, and_condition_string, bool>;

        using super::super;
    };

    struct or_condition_string {
        serialize_result_type serialize() const {
            return "OR";
        }
    };

    /**
     *  Result of or operator
     */
    template<class L, class R>
    struct or_condition_t : binary_condition<L, R, or_condition_string, bool>, negatable_t {
        using super = binary_condition<L, R, or_condition_string, bool>;

        using super::super;
    };

    struct is_equal_string {
        serialize_result_type serialize() const {
            return "=";
        }
    };

    /**
     *  = and == operators object
     */
    template<class L, class R>
    struct is_equal_t : binary_condition<L, R, is_equal_string, bool>, negatable_t {
        using self = is_equal_t<L, R>;

        using binary_condition<L, R, is_equal_string, bool>::binary_condition;

        collate_t<self> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<self> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<self> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }

        named_collate<self> collate(std::string name) const {
            return {*this, std::move(name)};
        }

        template<class C>
        named_collate<self> collate() const {
            std::stringstream ss;
            ss << C::name() << std::flush;
            return {*this, ss.str()};
        }
    };

    template<class L, class R>
    struct is_equal_with_table_t : negatable_t {
        using left_type = L;
        using right_type = R;

        right_type rhs;

        is_equal_with_table_t(right_type rhs) : rhs(std::move(rhs)) {}
    };

    struct is_not_equal_string {
        serialize_result_type serialize() const {
            return "!=";
        }
    };

    /**
     *  != operator object
     */
    template<class L, class R>
    struct is_not_equal_t : binary_condition<L, R, is_not_equal_string, bool>, negatable_t {
        using self = is_not_equal_t<L, R>;

        using binary_condition<L, R, is_not_equal_string, bool>::binary_condition;

        collate_t<self> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<self> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<self> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct greater_than_string {
        serialize_result_type serialize() const {
            return ">";
        }
    };

    /**
     *  > operator object.
     */
    template<class L, class R>
    struct greater_than_t : binary_condition<L, R, greater_than_string, bool>, negatable_t {
        using self = greater_than_t<L, R>;

        using binary_condition<L, R, greater_than_string, bool>::binary_condition;

        collate_t<self> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<self> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<self> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct greater_or_equal_string {
        serialize_result_type serialize() const {
            return ">=";
        }
    };

    /**
     *  >= operator object.
     */
    template<class L, class R>
    struct greater_or_equal_t : binary_condition<L, R, greater_or_equal_string, bool>, negatable_t {
        using self = greater_or_equal_t<L, R>;

        using binary_condition<L, R, greater_or_equal_string, bool>::binary_condition;

        collate_t<self> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<self> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<self> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct less_than_string {
        serialize_result_type serialize() const {
            return "<";
        }
    };

    /**
     *  < operator object.
     */
    template<class L, class R>
    struct less_than_t : binary_condition<L, R, less_than_string, bool>, negatable_t {
        using self = less_than_t<L, R>;

        using binary_condition<L, R, less_than_string, bool>::binary_condition;

        collate_t<self> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<self> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<self> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct less_or_equal_string {
        serialize_result_type serialize() const {
            return "<=";
        }
    };

    /**
     *  <= operator object.
     */
    template<class L, class R>
    struct less_or_equal_t : binary_condition<L, R, less_or_equal_string, bool>, negatable_t {
        using self = less_or_equal_t<L, R>;

        using binary_condition<L, R, less_or_equal_string, bool>::binary_condition;

        collate_t<self> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<self> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<self> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct order_by_base {
        std::string _collate_argument;
        int _order = 0;  //  -1 = desc, 1 = asc, 0 = unspecified
    };

    struct order_by_string {
        operator std::string() const {
            return "ORDER BY";
        }
    };

    /**
     *  ORDER BY argument holder.
     */
    template<class O>
    struct order_by_t : order_by_base, order_by_string {
        using expression_type = O;
        using self = order_by_t<expression_type>;

        expression_type _expression;

        order_by_t(expression_type expression) : order_by_base(), _expression(std::move(expression)) {}

        self asc() const {
            auto res = *this;
            res._order = 1;
            return res;
        }

        self desc() const {
            auto res = *this;
            res._order = -1;
            return res;
        }

        self collate_binary() const {
            auto res = *this;
            res._collate_argument = collate_constraint_t::string_from_collate_argument(collate_argument::binary);
            return res;
        }

        self collate_nocase() const {
            auto res = *this;
            res._collate_argument = collate_constraint_t::string_from_collate_argument(collate_argument::nocase);
            return res;
        }

        self collate_rtrim() const {
            auto res = *this;
            res._collate_argument = collate_constraint_t::string_from_collate_argument(collate_argument::rtrim);
            return res;
        }

        self collate(std::string name) const {
            auto res = *this;
            res._collate_argument = std::move(name);
            return res;
        }

        template<class C>
        self collate() const {
            std::stringstream ss;
            ss << C::name() << std::flush;
            return this->collate(ss.str());
        }
    };

    /**
     *  ORDER BY pack holder.
     */
    template<class... Args>
    struct multi_order_by_t : order_by_string {
        using args_type = std::tuple<Args...>;

        args_type args;

        multi_order_by_t(args_type args_) : args{std::move(args_)} {}
    };

    struct dynamic_order_by_entry_t : order_by_base {
        std::string name;

        dynamic_order_by_entry_t(decltype(name) name_, std::string collate_argument_, int asc_desc_) :
            order_by_base{std::move(collate_argument_), asc_desc_}, name(std::move(name_)) {}
    };

    /**
     *  C - serializer context class
     */
    template<class C>
    struct dynamic_order_by_t : order_by_string {
        using context_t = C;
        using entry_t = dynamic_order_by_entry_t;
        using const_iterator = typename std::vector<entry_t>::const_iterator;

        dynamic_order_by_t(const context_t& context_) : context(context_) {}

        template<class O>
        void push_back(order_by_t<O> orderBy) {
            auto newContext = this->context;
            newContext.omit_table_name = false;
            auto columnName = serialize(orderBy._expression, newContext);
            this->entries.emplace_back(std::move(columnName), std::move(orderBy._collate_argument), orderBy._order);
        }

        const_iterator begin() const {
            return this->entries.begin();
        }

        const_iterator end() const {
            return this->entries.end();
        }

        void clear() {
            this->entries.clear();
        }

      protected:
        std::vector<entry_t> entries;
        context_t context;
    };

    template<class T>
    inline constexpr bool is_order_by_v =
        polyfill::disjunction<polyfill::is_specialization_of<T, order_by_t>,
                              polyfill::is_specialization_of<T, multi_order_by_t>,
                              polyfill::is_specialization_of<T, dynamic_order_by_t>>::value;

    template<class T>
    struct is_order_by : polyfill::bool_constant<is_order_by_v<T>> {};

    struct like_string {
        operator std::string() const {
            return "LIKE";
        }
    };

    /**
     *  LIKE operator object.
     */
    template<class A, class T, class E>
    struct like_t : condition_t, like_string, negatable_t {
        using self = like_t<A, T, E>;
        using arg_t = A;
        using pattern_t = T;
        using escape_t = E;

        arg_t arg;
        pattern_t pattern;
        optional_container<escape_t> arg3;  //  not escape cause escape exists as a function here

        like_t(arg_t arg_, pattern_t pattern_, optional_container<escape_t> escape_) :
            arg(std::move(arg_)), pattern(std::move(pattern_)), arg3(std::move(escape_)) {}

        template<class C>
        like_t<A, T, C> escape(C c) const {
            optional_container<C> newArg3{std::move(c)};
            return {std::move(this->arg), std::move(this->pattern), std::move(newArg3)};
        }
    };

    struct glob_string {
        operator std::string() const {
            return "GLOB";
        }
    };

    template<class A, class T>
    struct glob_t : condition_t, glob_string, negatable_t {
        using self = glob_t<A, T>;
        using arg_t = A;
        using pattern_t = T;

        arg_t arg;
        pattern_t pattern;

        glob_t(arg_t arg_, pattern_t pattern_) : arg(std::move(arg_)), pattern(std::move(pattern_)) {}
    };

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
    using is_any_join = mpl::invoke_t<mpl::disjunction<check_if<is_constrained_join>,
                                                       check_if_is_template<cross_join_t>,
                                                       check_if_is_template<natural_join_t>>,
                                      T>;

    template<class... Tables>
    struct from_t {
        using tuple_type = std::tuple<Tables...>;
    };

    template<class T>
    using is_from = polyfill::is_specialization_of<T, from_t>;

    template<class... TableExpr>
    struct from2_t {
        using tuple_type = std::tuple<TableExpr...>;

        tuple_type table_expressions;
    };

    template<class T>
    using is_from2 = polyfill::is_specialization_of<T, from2_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Explicit FROM function. Usage:
     *  `storage.select(&User::id, from<User>());`
     */
    template<class... Tables>
    constexpr internal::from_t<Tables...> from() {
        static_assert(sizeof...(Tables) > 0);
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Explicit FROM function. Usage:
     *  `storage.select(&User::id, from<"a"_alias.for_<User>>());`
     */
    template<orm_refers_to_recordset auto... recordsets>
    constexpr auto from() {
        return from<internal::auto_decay_table_ref_t<recordsets>...>();
    }
#endif

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /**
     *  Explicit FROM for an eponymous virtual table used as a table-valued function. Usage:
     *  `storage.select(asterisk<dbstat>(), from(dbstat_table("main", true)));`
     */
    template<class... TableExpr>
        requires ((orm_refers_to_recordset<TableExpr> || orm_table_valued_expression<TableExpr>) && ...)
    constexpr internal::from2_t<TableExpr...> from(TableExpr... tableExpressions) {
        return {{std::move(tableExpressions)...}};
    }
#else
    /**
     *  Explicit FROM for an eponymous virtual table used as a table-valued function. Usage:
     *  `storage.select(asterisk<dbstat>(), from(dbstat_table("main", true)));`
     */
    template<class... TableExpr>
    constexpr internal::from2_t<TableExpr...> from(TableExpr... tableExpressions) {
        static_assert(
            ((internal::is_referring_to_recordset_v<TableExpr> || internal::is_table_valued_expression_v<TableExpr>) &&
             ...));
        return {{std::move(tableExpressions)...}};
    }
#endif

    // Intentionally place operators for types classified as arithmetic or general operator arguments in the internal namespace
    // to facilitate ADL (Argument Dependent Lookup)
    namespace internal {
        template<
            class T,
            std::enable_if_t<polyfill::disjunction<std::is_base_of<negatable_t, T>, is_operator_argument<T>>::value,
                             bool> = true>
        constexpr negated_condition_t<T> operator!(T arg) {
            return {std::move(arg)};
        }

        template<class L,
                 class R,
                 std::enable_if_t<polyfill::disjunction<std::is_base_of<arithmetic_t, L>,
                                                        std::is_base_of<arithmetic_t, R>,
                                                        is_operator_argument<L>,
                                                        is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr less_than_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator<(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<polyfill::disjunction<std::is_base_of<arithmetic_t, L>,
                                                        std::is_base_of<arithmetic_t, R>,
                                                        is_operator_argument<L>,
                                                        is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr less_or_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator<=(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<polyfill::disjunction<std::is_base_of<arithmetic_t, L>,
                                                        std::is_base_of<arithmetic_t, R>,
                                                        is_operator_argument<L>,
                                                        is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr greater_than_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator>(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<polyfill::disjunction<std::is_base_of<arithmetic_t, L>,
                                                        std::is_base_of<arithmetic_t, R>,
                                                        is_operator_argument<L>,
                                                        is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr greater_or_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator>=(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<polyfill::disjunction<std::is_base_of<arithmetic_t, L>,
                                                        std::is_base_of<arithmetic_t, R>,
                                                        std::is_base_of<condition_t, L>,
                                                        std::is_base_of<condition_t, R>,
                                                        is_operator_argument<L>,
                                                        is_operator_argument<R>>::value
#ifndef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
                                      && !is_table_reference_v<L>
#endif
                                  ,
                                  bool> = true>
        constexpr is_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator==(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<polyfill::disjunction<std::is_base_of<arithmetic_t, L>,
                                                        std::is_base_of<arithmetic_t, R>,
                                                        std::is_base_of<condition_t, L>,
                                                        std::is_base_of<condition_t, R>,
                                                        is_operator_argument<L>,
                                                        is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr is_not_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator!=(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<polyfill::disjunction<std::is_base_of<condition_t, L>,
                                                        std::is_base_of<condition_t, R>,
                                                        is_operator_argument<L>,
                                                        is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr and_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator&&(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<
                     polyfill::disjunction<std::is_base_of<condition_t, L>, std::is_base_of<condition_t, R>>::value,
                     bool> = true>
        constexpr or_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator||(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }

        template<
            class L,
            class R,
            std::enable_if_t<polyfill::conjunction<
                                 polyfill::disjunction<std::is_base_of<conc_string, L>,
                                                       std::is_base_of<conc_string, R>,
                                                       is_operator_argument<L>,
                                                       is_operator_argument<R>>,
                                 // exclude conditions
                                 polyfill::negation<polyfill::disjunction<std::is_base_of<condition_t, L>,
                                                                          std::is_base_of<condition_t, R>>>>::value,
                             bool> = true>
        constexpr conc_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator||(L l, R r) {
            return {get_from_expression(std::forward<L>(l)), get_from_expression(std::forward<R>(r))};
        }
    }

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

    template<class L, class R>
    constexpr auto and_(L l, R r) {
        using namespace ::sqlite_orm::internal;
        return and_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>>{get_from_expression(std::forward<L>(l)),
                                                                               get_from_expression(std::forward<R>(r))};
    }

    template<class L, class R>
    constexpr auto or_(L l, R r) {
        using namespace ::sqlite_orm::internal;
        return or_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>>{get_from_expression(std::forward<L>(l)),
                                                                              get_from_expression(std::forward<R>(r))};
    }

    template<class L, class R>
    constexpr internal::is_equal_t<L, R> is_equal(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::is_equal_t<L, R> eq(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    /** 
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    template<class O, class R, std::enable_if_t<!internal::is_recordset_alias_v<O>, bool> = true>
    [[deprecated("Use the usual `is_equal` function to compare the hidden FTS5 'any' field or a field of your FTS "
                 "table instead")]]
    constexpr internal::is_equal_with_table_t<O, R> is_equal(R rhs) {
        return {std::move(rhs)};
    }

    template<class L, class R>
    constexpr internal::is_not_equal_t<L, R> is_not_equal(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::is_not_equal_t<L, R> ne(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::greater_than_t<L, R> greater_than(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::greater_than_t<L, R> gt(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::greater_or_equal_t<L, R> greater_or_equal(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::greater_or_equal_t<L, R> ge(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::less_than_t<L, R> less_than(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    /**
     *  [Deprecation notice] This function is deprecated and will be removed in v1.10. Use the accurately named function `less_than(...)` instead.
     */
    template<class L, class R>
    [[deprecated("Use the accurately named function `less_than(...)` instead")]] internal::less_than_t<L, R>
    lesser_than(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::less_than_t<L, R> lt(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::less_or_equal_t<L, R> less_or_equal(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    /**
     *  [Deprecation notice] This function is deprecated and will be removed in v1.10. Use the accurately named function `less_or_equal(...)` instead.
     */
    template<class L, class R>
    [[deprecated("Use the accurately named function `less_or_equal(...)` instead")]] internal::less_or_equal_t<L, R>
    lesser_or_equal(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::less_or_equal_t<L, R> le(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    /**
     *  ORDER BY column, column alias or expression
     *  
     *  Examples:
     *  storage.select(&User::name, order_by(&User::id))
     *  storage.select(as<colalias_a>(&User::name), order_by(get<colalias_a>()))
     */
    template<class O, internal::satisfies_not<std::is_base_of, integer_printer, type_printer<O>> = true>
    internal::order_by_t<O> order_by(O o) {
        return {std::move(o)};
    }

    /** 
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    [[deprecated("Use the hidden FTS5 rank column instead")]]
    inline internal::order_by_t<internal::rank_t> order_by(internal::rank_t expression) {
        return {std::move(expression)};
    }

    /**
     *  ORDER BY positional ordinal
     *  
     *  Examples:
     *  storage.select(&User::name, order_by(1))
     */
    template<class O, internal::satisfies<std::is_base_of, integer_printer, type_printer<O>> = true>
    internal::order_by_t<internal::literal_holder<O>> order_by(O o) {
        return {{std::move(o)}};
    }

    /**
     *  ORDER BY column1, column2
     *  Example: storage.get_all<Singer>(multi_order_by(order_by(&Singer::name).asc(), order_by(&Singer::gender).desc())
     */
    template<class... Args>
    internal::multi_order_by_t<Args...> multi_order_by(Args... args) {
        return {{std::forward<Args>(args)...}};
    }

    /**
     *  ORDER BY column1, column2
     *  Difference from `multi_order_by` is that `dynamic_order_by` can be changed at runtime using `push_back` member
     *  function Example:
     *  auto orderBy = dynamic_order_by(storage);
     *  if(someCondition) {
     *    orderBy.push_back(&User::id);
     *  } else {
     *    orderBy.push_back(&User::name);
     *    orderBy.push_back(&User::birthDate);
     *  }
     */
    template<class S>
    internal::dynamic_order_by_t<internal::serializer_context<typename S::db_objects_type>>
    dynamic_order_by(const S& storage) {
        return {obtain_db_objects(storage)};
    }

    /**
     *  X LIKE Y
     *  Example: storage.select(like(&User::name, "T%"))
     */
    template<class A, class T>
    internal::like_t<A, T, void> like(A a, T t) {
        return {std::move(a), std::move(t), {}};
    }

    /**
     *  X GLOB Y
     *  Example: storage.select(glob(&User::name, "*S"))
     */
    template<class A, class T>
    internal::glob_t<A, T> glob(A a, T t) {
        return {std::move(a), std::move(t)};
    }

    /**
     *  X LIKE Y ESCAPE Z
     *  Example: storage.select(like(&User::name, "T%", "%"))
     */
    template<class A, class T, class E>
    internal::like_t<A, T, E> like(A a, T t, E e) {
        return {std::move(a), std::move(t), {std::move(e)}};
    }
}
