#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
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

    struct Empty {};
    struct A {
        int id_;
        explicit constexpr A(int i) : id_(i) {}
    };

    struct NoDefault {
        NoDefault() = delete;
    };
}

TEST_CASE("tuple - variadic forward") {
    {
        mpl::tuple<MoveOnly> t(MoveOnly(0));
        REQUIRE(std::get<0>(t) == 0);
    }
    {
        mpl::tuple<MoveOnly, MoveOnly> t(MoveOnly(0), MoveOnly(1));
        REQUIRE(std::get<0>(t) == 0);
        REQUIRE(std::get<1>(t) == 1);
    }
    {
        mpl::tuple<MoveOnly, MoveOnly, MoveOnly> t(MoveOnly(0), MoveOnly(1), MoveOnly(2));
        REQUIRE(std::get<0>(t) == 0);
        REQUIRE(std::get<1>(t) == 1);
        REQUIRE(std::get<2>(t) == 2);
    }
    {
        constexpr mpl::tuple<Empty> t0{Empty()};
        (void)t0;
    }
    {
        constexpr mpl::tuple<A, A> t(3, 2);
        STATIC_REQUIRE(std::get<0>(t).id_ == 3);
    }
    {
        typedef mpl::tuple<MoveOnly, NoDefault> Tuple;

        STATIC_REQUIRE(!std::is_constructible<Tuple, MoveOnly>::value);

        STATIC_REQUIRE(std::is_constructible<Tuple, MoveOnly, NoDefault>::value);
    }
    {
        typedef mpl::tuple<MoveOnly, MoveOnly, NoDefault> Tuple;

        STATIC_REQUIRE(!std::is_constructible<Tuple, MoveOnly, MoveOnly>::value);

        STATIC_REQUIRE(std::is_constructible<Tuple, MoveOnly, MoveOnly, NoDefault>::value);
    }
    {
        typedef mpl::tuple<MoveOnly, NoDefault> Tuple;
        typedef mpl::tuple<MoveOnly, Tuple, MoveOnly, MoveOnly> NestedTuple;

        STATIC_REQUIRE(!std::is_constructible<NestedTuple, MoveOnly, MoveOnly, MoveOnly, MoveOnly>::value);

        STATIC_REQUIRE(std::is_constructible<NestedTuple, MoveOnly, Tuple, MoveOnly, MoveOnly>::value);
    }
    {
        typedef mpl::tuple<MoveOnly, int> Tuple;
        typedef mpl::tuple<MoveOnly, Tuple, MoveOnly, MoveOnly> NestedTuple;

        STATIC_REQUIRE(!std::is_constructible<NestedTuple, MoveOnly, MoveOnly, MoveOnly, MoveOnly>::value);

        STATIC_REQUIRE(std::is_constructible<NestedTuple, MoveOnly, Tuple, MoveOnly, MoveOnly>::value);
    }
}
