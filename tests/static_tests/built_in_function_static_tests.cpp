#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same_v, std::is_invocable_v
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <tuple>  //  std::tuple

using namespace sqlite_orm;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
using internal::built_in_function_call;
using internal::built_in_scalar_function;
using internal::is_built_in_function_v;
using internal::is_operator_argument_v;
using internal::operator""_builtin;

TEST_CASE("built-in function static") {
    struct User {
        int id = 0;
        std::string name;
    };

    SECTION("definition") {
        using lower_type = decltype(lower);

        STATIC_REQUIRE(orm_built_in_function<lower_type>);
        STATIC_REQUIRE(std::is_same_v<lower_type, const built_in_scalar_function<6, std::string(std::string_view)>>);
        STATIC_REQUIRE(std::is_same_v<lower_type::signature_tuple, std::tuple<std::string(std::string_view)>>);
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
        STATIC_REQUIRE(std::is_same_v<node_type::args_type, std::tuple<std::string User::*>>);
        STATIC_REQUIRE(is_built_in_function_v<node_type>);
        STATIC_REQUIRE(is_operator_argument_v<node_type>);
        STATIC_REQUIRE(internal::is_arithmetic_operand_v<node_type>);

        constexpr auto node = lower("abc");
        STATIC_REQUIRE(node.serialize() == "LOWER");
        STATIC_REQUIRE(std::get<0>(node.args) == std::string_view{"abc"});
    }
    SECTION("overload set") {
        STATIC_REQUIRE(orm_built_in_function<decltype(substr)>);
        STATIC_REQUIRE(
            std::is_same_v<decltype(substr)::signature_tuple,
                           std::tuple<std::string(std::string_view, int), std::string(std::string_view, int, int)>>);
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
        constexpr auto max_f = "MAX"_builtin.scalar<double(double, double, variadic<double>)>();

        STATIC_REQUIRE(!std::is_invocable_v<decltype(max_f)>);
        STATIC_REQUIRE(!std::is_invocable_v<decltype(max_f), int>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(max_f), int, int>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(max_f), int, int, int>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(max_f), int, int, int, int, int>);
        STATIC_REQUIRE(std::is_same_v<decltype(max_f(1, 2, 3))::return_type, double>);

        // zero or more: a lone marker accepts an empty call
        constexpr auto any_f = "ANY"_builtin.scalar<int(variadic<int>)>();
        STATIC_REQUIRE(std::is_invocable_v<decltype(any_f)>);
        STATIC_REQUIRE(std::is_invocable_v<decltype(any_f), int, int>);
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
