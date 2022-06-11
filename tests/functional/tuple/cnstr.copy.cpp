#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    struct Empty {};
}

template<template<class...> class Tuple>
static void template_template_test_case() {
    {
        using T = Tuple<>;
        T t0;
        T t_implicit = t0;
        T t_explicit(t0);

        (void)t_explicit;
        (void)t_implicit;
    }
    {
        using T = Tuple<int>;
        T t0(2);
        T t = t0;
        REQUIRE(std::get<0>(t) == 2);
    }
    {
        using T = Tuple<int, char>;
        T t0(2, 'a');
        T t = t0;
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == 'a');
    }
    {
        using T = Tuple<int, char, std::string>;
        const T t0(2, 'a', "some text");
        T t = t0;
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == 'a');
        REQUIRE(std::get<2>(t) == "some text");
    }
    {
        using T = Tuple<int>;
        constexpr T t0(2);
        constexpr T t = t0;
        static_assert(std::get<0>(t) == 2, "");
    }
    {
        using T = Tuple<Empty>;
        constexpr T t0;
        constexpr T t = t0;
        constexpr Empty e = std::get<0>(t0);
        (void)e;
    }
    {
        struct T {};
        struct U {};

        constexpr Tuple<T, U> binary{};
        constexpr Tuple<T, U> copy_implicit = binary;
        constexpr Tuple<T, U> copy_explicit(binary);

        (void)copy_implicit;
        (void)copy_explicit;
    }
}

TEST_CASE("tuple - copy") {
    {
        INFO("mpl::tuple");
        template_template_test_case<mpl::tuple>();
    }
    {
        INFO("mpl::uple");
        template_template_test_case<mpl::uple>();
    }
}
