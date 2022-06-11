#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    struct DefaultOnly {
        int data_;
        DefaultOnly(DefaultOnly const&) = delete;
        DefaultOnly& operator=(DefaultOnly const&) = delete;

        static int count;

        DefaultOnly() : data_(-1) {
            ++count;
        }
        ~DefaultOnly() {
            data_ = 0;
            --count;
        }

        friend bool operator==(DefaultOnly const& x, DefaultOnly const& y) {
            return x.data_ == y.data_;
        }

        friend bool operator<(DefaultOnly const& x, DefaultOnly const& y) {
            return x.data_ < y.data_;
        }
    };

    int DefaultOnly::count = 0;

    struct NoDefault {
        NoDefault() = delete;
        explicit NoDefault(int) {}
    };

    struct IllFormedDefault {
        IllFormedDefault(int x) : value(x) {}
        template<bool Pred = false>
        constexpr IllFormedDefault() {
            static_assert(Pred, "The default constructor should not be instantiated");
        }
        int value;
    };
}

template<template<class...> class Tuple>
static void template_template_test_case() {
    {
        Tuple<> t;
        (void)t;
    }
    {
        Tuple<int> t;
        REQUIRE(std::get<0>(t) == 0);
    }
    {
        Tuple<int, char*> t;
        REQUIRE(std::get<0>(t) == 0);
        REQUIRE(std::get<1>(t) == nullptr);
    }
    {
        Tuple<int, char*, std::string> t;
        REQUIRE(std::get<0>(t) == 0);
        REQUIRE(std::get<1>(t) == nullptr);
        REQUIRE(std::get<2>(t) == "");
    }
    {
        Tuple<int, char*, std::string, DefaultOnly> t;
        REQUIRE(std::get<0>(t) == 0);
        REQUIRE(std::get<1>(t) == nullptr);
        REQUIRE(std::get<2>(t) == "");
        REQUIRE(std::get<3>(t) == DefaultOnly());
    }
    {
        // See LLVM bug #21157.
        STATIC_REQUIRE(!std::is_default_constructible<Tuple<NoDefault>>());
        STATIC_REQUIRE(!std::is_default_constructible<Tuple<DefaultOnly, NoDefault>>());
    }
    {
        struct T {};
        struct U {};
        struct V {};

        constexpr Tuple<> z0;
        (void)z0;
        constexpr Tuple<T> z1;
        (void)z1;
        constexpr Tuple<T, U> z2;
        (void)z2;
        constexpr Tuple<T, U, V> z3;
        (void)z3;
    }
    {
        constexpr Tuple<int> t;
        REQUIRE(std::get<0>(t) == 0);
    }
    {
        constexpr Tuple<int, char*> t;
        REQUIRE(std::get<0>(t) == 0);
        REQUIRE(std::get<1>(t) == nullptr);
    }

    // Make sure we can hold non default-constructible elements, and that
    // it does not trigger an error in the default constructor.
    {
        IllFormedDefault v(0);
        Tuple<IllFormedDefault> t1(v);
        Tuple<IllFormedDefault> t2{v};
        Tuple<IllFormedDefault> t3 = {v};
        (void)t1;
        (void)t2;
        (void)t3;
    }
    {
        Tuple<NoDefault> t1(0);
        Tuple<NoDefault> t2{0};
        Tuple<NoDefault> t3 = {0};
        (void)t1;
        (void)t2;
        (void)t3;
    }
    {
        NoDefault v(0);
        Tuple<NoDefault> t1(v);
        Tuple<NoDefault> t2{v};
        Tuple<NoDefault> t3 = {v};
        (void)t1;
        (void)t2;
        (void)t3;
    }
}

TEST_CASE("tuple - default") {
    {
        INFO("mpl::tuple");
        template_template_test_case<mpl::tuple>();
    }
    {
        INFO("mpl::uple");
        template_template_test_case<mpl::uple>();
    }
}
