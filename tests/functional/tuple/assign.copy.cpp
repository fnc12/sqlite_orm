#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

template<template<class...> class Tuple>
static void template_template_test_case() {
    {
        using T = Tuple<>;
        T t0;
        T t;
        t = t0;
    }
    {
        using T = Tuple<int>;
        T t0(2);
        T t;
        t = t0;
        REQUIRE(std::get<0>(t) == 2);
    }
    {
        using T = Tuple<int, char>;
        T t0(2, 'a');
        T t;
        t = t0;
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == 'a');
    }
    {
        using T = Tuple<int, char, std::string>;
        const T t0(2, 'a', "some text");
        T t;
        t = t0;
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == 'a');
        REQUIRE(std::get<2>(t) == "some text");
    }
}

TEST_CASE("tuple - copy assignment") {
    {
        INFO("mpl::tuple");
        template_template_test_case<mpl::tuple>();
    }
    {
        INFO("mpl::uple");
        template_template_test_case<mpl::uple>();
    }
}
