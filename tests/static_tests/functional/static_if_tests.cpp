#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
using internal::call_if_constexpr;
using internal::static_if;

TEST_CASE("static_if") {
    using called_pair = std::pair<int, int>;
    called_pair called;
    {  //  simple true
        called = {};
        static_if<std::true_type::value>(
            [&called] {
                ++called.first;
            },
            [&called] {
                ++called.second;
            })();
        REQUIRE(called == called_pair{1, 0});
    }
    {  //  simple false
        called = {};
        static_if<std::false_type::value>(
            [&called] {
                ++called.first;
            },
            [&called] {
                ++called.second;
            })();
        REQUIRE(called == called_pair{0, 1});
    }
    {  //  tuple is empty
        called = {};
        static_if<std::is_empty<std::tuple<>>::value>(
            [&called] {
                ++called.first;
            },
            [&called] {
                ++called.second;
            })();
        REQUIRE(called == called_pair{1, 0});
    }
    {  //  tuple is not empty
        called = {};
        static_if<polyfill::negation_v<std::is_empty<std::tuple<>>>>(
            [&called] {
                ++called.first;
            },
            [&called] {
                ++called.second;
            })();
        REQUIRE(called == called_pair{0, 1});
    }
    {
        struct User {
            std::string name;
        };
        auto ch = check(length(&User::name) > 5);
        STATIC_REQUIRE(!internal::is_column_v<decltype(ch)>);
        called = {};
        static_if<internal::is_column_v<decltype(ch)>>(
            [&called] {
                ++called.first;
            },
            [&called] {
                ++called.second;
            })();
        REQUIRE(called == called_pair{0, 1});
    }
    {  //  simple true
        called = {};
        call_if_constexpr<std::true_type::value>([&called] {
            ++called.first;
        });
        REQUIRE(called == called_pair{1, 0});
    }
    {  //  simple false
        called = {};
        call_if_constexpr<std::false_type::value>([&called] {
            ++called.first;
        });
        REQUIRE(called == called_pair{0, 0});
    }
}
