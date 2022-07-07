#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    struct A {
        int id_;
        constexpr A(int i) : id_(i) {}
        friend constexpr bool operator==(const A& x, const A& y) {
            return x.id_ == y.id_;
        }
    };

    struct B {
        int id_;
        explicit B(int i) : id_(i) {}
    };

    struct C {
        int id_;
        constexpr explicit C(int i) : id_(i) {}
        friend constexpr bool operator==(const C& x, const C& y) {
            return x.id_ == y.id_;
        }
    };

    struct D : B {
        explicit D(int i) : B(i) {}
    };
}

template<template<class...> class Tuple>
static void template_template_test_case() {
    {
        using T0 = Tuple<double>;
        using T1 = Tuple<int>;
        T0 t0(2.5);
        T1 t1 = t0;
        REQUIRE(std::get<0>(t1) == 2);
    }
    {
        using T0 = Tuple<double>;
        using T1 = Tuple<A>;
        constexpr T0 t0(2.5);
        constexpr T1 t1 = t0;
        STATIC_REQUIRE(std::get<0>(t1) == 2);
    }
    {
        using T0 = Tuple<int>;
        using T1 = Tuple<C>;
        constexpr T0 t0(2);
        constexpr T1 t1{t0};
        STATIC_REQUIRE(std::get<0>(t1) == C(2));
    }
    {
        using T0 = Tuple<double, char>;
        using T1 = Tuple<int, unsigned int>;
        T0 t0(2.5, 'a');
        T1 t1 = t0;
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
    }
    {
        using T0 = Tuple<double, char, D>;
        using T1 = Tuple<int, unsigned int, B>;
        T0 t0(2.5, 'a', D(3));
        T1 t1 = t0;
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
        REQUIRE(std::get<2>(t1).id_ == 3);
    }
    {
        D d(3);
        using T0 = Tuple<double, char, D&>;
        using T1 = Tuple<int, unsigned int, B&>;
        T0 t0(2.5, 'a', d);
        T1 t1 = t0;
        d.id_ = 2;
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
        REQUIRE(std::get<2>(t1).id_ == 2);
    }
    {
        using T0 = Tuple<double, char, int>;
        using T1 = Tuple<int, unsigned int, B>;
        T0 t0(2.5, 'a', 3);
        T1 t1(t0);
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
        REQUIRE(std::get<2>(t1).id_ == 3);
    }
}

TEST_CASE("tuple - converting copy construction") {
    {
        INFO("mpl::tuple");
        template_template_test_case<mpl::tuple>();
    }
    {
        INFO("mpl::uple");
        template_template_test_case<mpl::uple>();
    }
}
