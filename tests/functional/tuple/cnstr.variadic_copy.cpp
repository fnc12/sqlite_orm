#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    template<typename...>
    struct never {
        static constexpr bool value = false;
    };

    struct NoValueCtor {
        NoValueCtor() : id(++count) {}
        NoValueCtor(NoValueCtor const& other) : id(other.id) {
            ++count;
        }

        // The constexpr is required to make is_constructible instantiate this
        // template. The explicit is needed to test-around a similar bug with
        // is_convertible.
        template<typename T>
        constexpr explicit NoValueCtor(T) {
            static_assert(never<T>::value, "This should not be instantiated");
        }

        static int count;
        int id;
    };

    int NoValueCtor::count = 0;

    struct NoValueCtorEmpty {
        NoValueCtorEmpty() {}
        NoValueCtorEmpty(NoValueCtorEmpty const&) {}

        template<typename T>
        constexpr explicit NoValueCtorEmpty(T) {
            static_assert(never<T>::value, "This should not be instantiated");
        }
    };
}

template<template<class...> class Tuple>
static void template_template_test_case() {
    {
        Tuple<int> t(2);
        REQUIRE(std::get<0>(t) == 2);
    }
    {
        constexpr Tuple<int> t(2);
        STATIC_REQUIRE(std::get<0>(t) == 2);
    }
    {
        constexpr Tuple<int> t;
        STATIC_REQUIRE(std::get<0>(t) == 0);
    }
    {
        constexpr Tuple<int, char*> t(2, nullptr);
        STATIC_REQUIRE(std::get<0>(t) == 2);
        STATIC_REQUIRE(std::get<1>(t) == nullptr);
    }
    {
        Tuple<int, char*> t(2, nullptr);
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == nullptr);
    }
    {
        Tuple<int, char*, std::string> t(2, nullptr, "text");
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == nullptr);
        REQUIRE(std::get<2>(t) == "text");
    }
    {
        Tuple<int, NoValueCtor, long, unsigned int> t(1, NoValueCtor(), 2, 3);
        REQUIRE(std::get<0>(t) == 1);
        REQUIRE(std::get<1>(t).id == NoValueCtor::count - 1);
        REQUIRE(std::get<2>(t) == 2);
        REQUIRE(std::get<3>(t) == 3);
    }
    {
        Tuple<int, NoValueCtorEmpty, long, unsigned int> t(1, NoValueCtorEmpty(), 2, 3);
        REQUIRE(std::get<0>(t) == 1);
        REQUIRE(std::get<2>(t) == 2);
        REQUIRE(std::get<3>(t) == 3);
    }
    {
        struct T {};
        struct U {};
        struct V {};

        constexpr T t{};
        constexpr U u{};
        constexpr V v{};

        constexpr Tuple<T> x1{t};
        (void)x1;
        constexpr Tuple<T, U> x2{t, u};
        (void)x2;
        constexpr Tuple<T, U, V> x3{t, u, v};
        (void)x3;
    }
    {
        struct T {};
        struct U {};
        struct V {};

        // Check for SFINAE-friendliness
        STATIC_REQUIRE(!std::is_constructible<Tuple<T, U>, T>{});

        STATIC_REQUIRE(!std::is_constructible<Tuple<T, U>, U, T>{});

        STATIC_REQUIRE(!std::is_constructible<Tuple<T, U>, T, U, V>{});
    }

    // Make sure we can initialize elements with the brace-init syntax.
    {
        struct Member {};
        struct Element {
            Member member;
        };
        struct Element2 {
            Member member;
        };
        struct Element3 {
            Member member;
        };

        Tuple<Element, Element2, Element3> xs{{Member()}, {Member()}, {Member()}};
        Tuple<Element, Element2, Element3> ys = {{Member()}, {Member()}, {Member()}};
        (void)xs;
        (void)ys;
    }
}

TEST_CASE("tuple - variadic copy") {
    {
        INFO("mpl::tuple");
        template_template_test_case<mpl::tuple>();
    }
    {
        INFO("mpl::uple");
        template_template_test_case<mpl::uple>();
    }
}
