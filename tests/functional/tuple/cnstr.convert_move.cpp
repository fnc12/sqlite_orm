#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    struct B {
        int id_;
        explicit B(int i) : id_(i) {}
        virtual ~B() {}
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
        T1 t1 = std::move(t0);
        REQUIRE(std::get<0>(t1) == 2);
    }
    {
        using T0 = Tuple<double, char>;
        using T1 = Tuple<int, unsigned int>;
        T0 t0(2.5, 'a');
        T1 t1 = std::move(t0);
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
    }
    {
        using T0 = Tuple<double, char, D>;
        using T1 = Tuple<int, unsigned int, B>;
        T0 t0(2.5, 'a', D(3));
        T1 t1 = std::move(t0);
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
        REQUIRE(std::get<2>(t1).id_ == 3);
    }
    {
        D d(3);
        using T0 = Tuple<double, char, D&>;
        using T1 = Tuple<int, unsigned int, B&>;
        T0 t0(2.5, 'a', d);
        T1 t1 = std::move(t0);
        d.id_ = 2;
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
        REQUIRE(std::get<2>(t1).id_ == 2);
    }
    {
        using T0 = Tuple<double, char, std::unique_ptr<D>>;
        using T1 = Tuple<int, unsigned int, std::unique_ptr<B>>;
        T0 t0(2.5, 'a', std::unique_ptr<D>(new D(3)));
        T1 t1 = std::move(t0);
        REQUIRE(std::get<0>(t1) == 2);
        REQUIRE(std::get<1>(t1) == int('a'));
        REQUIRE(std::get<2>(t1)->id_ == 3);
    }
}

TEST_CASE("tuple - converting move construction") {
    {
        INFO("mpl::tuple");
        template_template_test_case<mpl::tuple>();
    }
    {
        INFO("mpl::uple");
        template_template_test_case<mpl::uple>();
    }
}
