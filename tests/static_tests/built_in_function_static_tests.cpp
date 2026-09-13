#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same_v, std::is_invocable_v
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <tuple>  //  std::tuple
#include <memory>  //  std::unique_ptr
#include <optional>  //  std::optional
#include <utility>  //  std::move

using namespace sqlite_orm;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
using internal::aggregate_sig;
using internal::anything;
using internal::argument;
using internal::built_in_aggregate_function_call;
using internal::built_in_function;
using internal::built_in_function_call;
using internal::column_result_of_t;
using internal::db_objects_tuple;
using internal::filtered_aggregate_function;
using internal::is_built_in_function_v;
using internal::is_operator_argument_v;
using internal::over_t;
using internal::scalar_sig;
using internal::variadic;
using internal::operator""_builtin;

template<class Node, class Where>
concept filterable = requires(Node node, Where wh) { node.filter(std::move(wh)); };

template<class Node>
concept overable = requires(Node node) { node.over(); };

template<class F, class R, class... Args>
concept callable_as = requires(const F& f, Args... args) { f.template operator()<R>(args...); };

TEST_CASE("built-in function static") {
    struct User {
        int id = 0;
        std::string name;
    };

    SECTION("definition") {
        using lower_type = decltype(lower);

        STATIC_REQUIRE(orm_built_in_function<lower_type>);
        STATIC_REQUIRE(
            std::is_same_v<lower_type, const built_in_function<6, scalar_sig<std::string(std::string_view)>>>);
        STATIC_REQUIRE(
            std::is_same_v<lower_type::signature_tuple, std::tuple<scalar_sig<std::string(std::string_view)>>>);
        STATIC_REQUIRE(lower.name() == "LOWER");
        STATIC_REQUIRE(lower.name().size() == 5);
        STATIC_REQUIRE(!std::is_invocable_v<lower_type>);
        STATIC_REQUIRE(!std::is_invocable_v<lower_type, std::string User::*, int>);
    }
    SECTION("call node") {
        using node_type = decltype(lower(&User::name));

        STATIC_REQUIRE(std::is_same_v<node_type,
                                      built_in_function_call<std::remove_const_t<decltype(lower)>,
                                                             std::string(std::string_view),
                                                             std::string User::*>>);
        STATIC_REQUIRE(std::is_same_v<node_type::return_type, std::string>);
        STATIC_REQUIRE(std::is_same_v<node_type::args_tuple, std::tuple<std::string User::*>>);
        STATIC_REQUIRE(is_built_in_function_v<node_type>);
        STATIC_REQUIRE(is_operator_argument_v<node_type>);
        STATIC_REQUIRE(internal::is_arithmetic_operand_v<node_type>);

        constexpr auto node = lower("abc");
        STATIC_REQUIRE(node.serialize() == "LOWER");
        STATIC_REQUIRE(std::get<0>(node.args) == std::string_view{"abc"});
    }
    SECTION("overload set") {
        STATIC_REQUIRE(orm_built_in_function<decltype(substr)>);
        STATIC_REQUIRE(std::is_same_v<decltype(substr)::signature_tuple,
                                      std::tuple<scalar_sig<std::string(std::string_view, int)>,
                                                 scalar_sig<std::string(std::string_view, int, int)>>>);
        STATIC_REQUIRE(
            std::is_same_v<decltype(substr(&User::name, 1))::signature_type, std::string(std::string_view, int)>);
        STATIC_REQUIRE(std::is_same_v<decltype(substr(&User::name, 1, 2))::signature_type,
                                      std::string(std::string_view, int, int)>);
        STATIC_REQUIRE(std::is_same_v<decltype(substr(&User::name, 1, 2))::return_type, std::string>);
        STATIC_REQUIRE(!std::is_invocable_v<decltype(substr), std::string User::*>);
        STATIC_REQUIRE(!std::is_invocable_v<decltype(substr), std::string User::*, int, int, int>);
#if SQLITE_VERSION_NUMBER >= 3034000
        // SUBSTRING aliases SUBSTR
        STATIC_REQUIRE(std::is_same_v<decltype(substring)::signature_tuple, decltype(substr)::signature_tuple>);
        STATIC_REQUIRE(substring.name() == "SUBSTRING");
#endif
    }
    SECTION("return type follows the matched overload") {
        constexpr auto f = "F"_builtin.scalar<int(int), double(int, int)>();

        STATIC_REQUIRE(std::is_same_v<decltype(f(1))::return_type, int>);
        STATIC_REQUIRE(std::is_same_v<decltype(f(1, 2))::return_type, double>);
    }
    SECTION("open-ended arity") {
        constexpr auto sum_f = "SUM"_builtin.scalar<double(double, double, variadic<double>)>();

        STATIC_REQUIRE(!std::is_invocable_v<decltype(sum_f)>);
        STATIC_REQUIRE(!std::is_invocable_v<decltype(sum_f), int>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(sum_f), int, int>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(sum_f), int, int, int>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(sum_f), int, int, int, int, int>);
        STATIC_REQUIRE(std::is_same_v<decltype(sum_f(1, 2, 3))::return_type, double>);

        // zero or more: a lone marker accepts an empty call
        constexpr auto any_f = "ANY"_builtin.scalar<int(variadic<int>)>();
        STATIC_REQUIRE(std::is_invocable_v<decltype(any_f)>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(any_f), int, int>);
    }
    SECTION("kinded dispatch") {
        using max_type = decltype(max);
        using aggregate_sig_type = std::unique_ptr<argument<0>>(anything);
        using scalar_sig_type = std::unique_ptr<argument<0>>(anything, anything, variadic<anything>);

        STATIC_REQUIRE(orm_built_in_function<max_type>);
        STATIC_REQUIRE(std::is_same_v<max_type::signature_tuple,
                                      std::tuple<aggregate_sig<aggregate_sig_type>, scalar_sig<scalar_sig_type>>>);
        STATIC_REQUIRE(std::is_same_v<decltype(min)::signature_tuple, max_type::signature_tuple>);

        // one argument: the aggregate
        using aggregate_node = decltype(max(&User::id));
        STATIC_REQUIRE(
            std::is_same_v<
                aggregate_node,
                built_in_aggregate_function_call<std::remove_const_t<max_type>, aggregate_sig_type, int User::*>>);
        STATIC_REQUIRE(std::is_same_v<aggregate_node::signature_type, aggregate_sig_type>);
        STATIC_REQUIRE(is_built_in_function_v<aggregate_node>);
        STATIC_REQUIRE(is_operator_argument_v<aggregate_node>);
        STATIC_REQUIRE(internal::is_arithmetic_operand_v<aggregate_node>);

        // two or more: the scalar
        using scalar_node = decltype(max(&User::id, 4));
        STATIC_REQUIRE(
            std::is_same_v<scalar_node,
                           built_in_function_call<std::remove_const_t<max_type>, scalar_sig_type, int User::*, int>>);
        STATIC_REQUIRE(std::is_same_v<decltype(max(1, 2, 3, 4, 5))::signature_type, scalar_sig_type>);
        STATIC_REQUIRE(!std::is_invocable_v<max_type>);

        constexpr auto node = max(1, 2);
        STATIC_REQUIRE(node.serialize() == "MAX");
    }
    SECTION("aggregate node") {
        using aggregate_node = decltype(max(&User::id));
        using scalar_node = decltype(max(&User::id, 4));

        STATIC_REQUIRE(std::is_base_of_v<built_in_function_call<std::remove_const_t<decltype(max)>,
                                                                std::unique_ptr<argument<0>>(anything),
                                                                int User::*>,
                                         aggregate_node>);
        STATIC_REQUIRE(
            std::is_same_v<decltype(max(&User::id).filter(where(c(&User::id) > 1))),
                           filtered_aggregate_function<aggregate_node, internal::greater_than_t<int User::*, int>>>);
        STATIC_REQUIRE(std::is_same_v<decltype(max(&User::id).over()), over_t<aggregate_node>>);
        STATIC_REQUIRE(std::is_same_v<decltype(min(&User::id).over(partition_by(&User::name))),
                                      over_t<decltype(min(&User::id)), internal::partition_by_t<std::string User::*>>>);
        // the scalar node has neither
        using where_type = decltype(where(c(&User::id) > 1));
        STATIC_REQUIRE(filterable<aggregate_node, where_type>);
        STATIC_REQUIRE(overable<aggregate_node>);
        STATIC_REQUIRE(!filterable<scalar_node, where_type>);
        STATIC_REQUIRE(!overable<scalar_node>);
    }
    SECTION("argument placeholder") {
        using dbos = db_objects_tuple<>;

        // MAX/MIN: nullable, typed like the first argument
        STATIC_REQUIRE(std::is_same_v<column_result_of_t<dbos, decltype(max(&User::id))>, std::unique_ptr<int>>);
        STATIC_REQUIRE(
            std::is_same_v<column_result_of_t<dbos, decltype(max(&User::name))>, std::unique_ptr<std::string>>);
        STATIC_REQUIRE(std::is_same_v<column_result_of_t<dbos, decltype(min(&User::id, 4))>, std::unique_ptr<int>>);
        STATIC_REQUIRE(std::is_same_v<column_result_of_t<dbos, decltype(max(4, &User::id))>, std::unique_ptr<int>>);
        STATIC_REQUIRE(
            std::is_same_v<column_result_of_t<dbos, decltype(max(&User::id).filter(where(c(&User::id) > 1)))>,
                           std::unique_ptr<int>>);

        // bare placeholder, and one nested somewhere else than the first argument
        constexpr auto same_f = "SAME"_builtin.scalar<argument<0>(anything)>();
        STATIC_REQUIRE(std::is_same_v<column_result_of_t<dbos, decltype(same_f(&User::name))>, std::string>);
        constexpr auto second_f = "SECOND"_builtin.scalar<std::optional<argument<1>>(anything, anything)>();
        STATIC_REQUIRE(
            std::is_same_v<column_result_of_t<dbos, decltype(second_f(1, &User::name))>, std::optional<std::string>>);

        // no placeholder: the declared type as is
        STATIC_REQUIRE(std::is_same_v<column_result_of_t<dbos, decltype(lower(&User::name))>, std::string>);
    }
    SECTION("return type override") {
        // an explicit `R` replaces the declared return type
        using node_type = decltype(lower.template operator()<std::optional<std::string>>(&User::name));
        STATIC_REQUIRE(std::is_same_v<node_type,
                                      built_in_function_call<std::remove_const_t<decltype(lower)>,
                                                             std::optional<std::string>(std::string_view),
                                                             std::string User::*>>);
        STATIC_REQUIRE(std::is_same_v<node_type::return_type, std::optional<std::string>>);
        STATIC_REQUIRE(std::is_same_v<column_result_of_t<db_objects_tuple<>, node_type>, std::optional<std::string>>);
        // the overload is still picked by arity
        STATIC_REQUIRE(
            std::is_same_v<decltype(substr.template operator()<std::string_view>(&User::name, 1, 2))::signature_type,
                           std::string_view(std::string_view, int, int)>);
        STATIC_REQUIRE(callable_as<decltype(lower), int, std::string User::*>);
        STATIC_REQUIRE(!callable_as<decltype(lower), int>);
        // and a placeholder in the override is substituted like a declared one
        STATIC_REQUIRE(std::is_same_v<
                       column_result_of_t<db_objects_tuple<>,
                                          decltype(lower.template operator()<std::optional<argument<0>>>(&User::id))>,
                       std::optional<int>>);
#ifdef SQLITE_ENABLE_MATH_FUNCTIONS
        // function template facade over a definition object, keeping the `<R>` spelling
        STATIC_REQUIRE(orm_built_in_function<decltype(internal::acos)>);
        STATIC_REQUIRE(
            std::is_same_v<decltype(sqlite_orm::acos(1)),
                           built_in_function_call<std::remove_const_t<decltype(internal::acos)>, double(double), int>>);
        STATIC_REQUIRE(
            std::is_same_v<decltype(sqlite_orm::acos<std::optional<double>>(1))::return_type, std::optional<double>>);
        STATIC_REQUIRE(std::is_same_v<decltype(sqlite_orm::acos<float>(&User::id))::signature_type, float(double)>);
        STATIC_REQUIRE(is_built_in_function_v<decltype(sqlite_orm::acos(1))>);
#endif
    }
    SECTION("first match in declaration order") {
        constexpr auto f = "F"_builtin.scalar<int(int, variadic<int>), double(int, int)>();

        // the open-ended overload comes first and accepts 2 arguments, so it wins
        STATIC_REQUIRE(std::is_same_v<decltype(f(1, 2))::signature_type, int(int, variadic<int>)>);

        constexpr auto g = "G"_builtin.scalar<double(int, int), int(int, variadic<int>)>();
        STATIC_REQUIRE(std::is_same_v<decltype(g(1, 2))::signature_type, double(int, int)>);
        STATIC_REQUIRE(std::is_same_v<decltype(g(1, 2, 3))::signature_type, int(int, variadic<int>)>);
    }
}
#endif
