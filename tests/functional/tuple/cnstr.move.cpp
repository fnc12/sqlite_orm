#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    template<int x>
    struct MoveOnly {
        int data_;
        MoveOnly(MoveOnly const&) = delete;
        MoveOnly& operator=(MoveOnly const&) = delete;
        MoveOnly(int data = 1) : data_(data) {}
        MoveOnly(MoveOnly&& x) : data_(x.data_) {
            x.data_ = 0;
        }

        MoveOnly& operator=(MoveOnly&& x) {
            data_ = x.data_;
            x.data_ = 0;
            return *this;
        }

        int get() const {
            return data_;
        }
        bool operator==(const MoveOnly& x) const {
            return data_ == x.data_;
        }
        bool operator<(const MoveOnly& x) const {
            return data_ < x.data_;
        }
    };

    using MoveOnly1 = MoveOnly<1>;
    using MoveOnly2 = MoveOnly<2>;
}

template<template<class...> class Tuple>
static void template_template_test_case() {
    {
        using T = mpl::tuple<>;
        T t0;
        T t = std::move(t0);
        (void)t;
    }
    {
        using T = mpl::tuple<MoveOnly1>;
        T t0(MoveOnly1(0));
        T t = std::move(t0);
        (void)t;
        REQUIRE(std::get<0>(t) == 0);
    }
    {
        using T = mpl::tuple<MoveOnly1, MoveOnly2>;
        T t0(MoveOnly1(0), MoveOnly2(1));
        T t = std::move(t0);
        (void)t;
        REQUIRE(std::get<0>(t) == 0);
        REQUIRE(std::get<1>(t) == 1);
    }
    {
        // Check for SFINAE-friendliness
        STATIC_REQUIRE(!std::is_constructible<mpl::tuple<MoveOnly1>, MoveOnly1 const&>{});

        STATIC_REQUIRE(!std::is_constructible<mpl::tuple<MoveOnly1>, MoveOnly1&>{});

        STATIC_REQUIRE(std::is_constructible<mpl::tuple<MoveOnly1>, MoveOnly1>{});

        STATIC_REQUIRE(std::is_constructible<mpl::tuple<MoveOnly2>, MoveOnly2&&>{});
    }
}

TEST_CASE("tuple - move") {
    {
        INFO("mpl::tuple");
        template_template_test_case<mpl::tuple>();
    }
    {
        INFO("mpl::uple");
        template_template_test_case<mpl::uple>();
    }
}
