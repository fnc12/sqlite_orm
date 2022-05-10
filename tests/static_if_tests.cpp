#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
using internal::call_if_constexpr;
using internal::static_if;

TEST_CASE("static_if") {
    {  //  simple true
        auto value = 0;
        static_if<std::true_type::value>(
            [&value] {
                value = 1;
            },
            [&value] {
                value = -1;
            })();
        REQUIRE(value == 1);
    }
    {  //  simple false
        auto value = 0;
        static_if<std::false_type::value>(
            [&value] {
                value = 1;
            },
            [&value] {
                value = -1;
            })();
        REQUIRE(value == -1);
    }
    {  //  tuple is empty
        auto value = 0;
        static_if<std::is_empty<std::tuple<>>::value>(
            [&value] {
                value = 1;
            },
            [&value] {
                value = -1;
            })();
        REQUIRE(value == 1);
    }
    {  //  tuple is not empty
        auto value = 0;
        static_if<polyfill::negation_v<std::is_empty<std::tuple<>>>>(
            [&value] {
                value = 1;
            },
            [&value] {
                value = -1;
            })();
        REQUIRE(value == -1);
    }
    {
        struct User {
            std::string name;
        };
        auto ch = check(length(&User::name) > 5);
        STATIC_REQUIRE(!internal::is_column_v<decltype(ch)>);
        int called = 0;
        static_if<internal::is_column_v<decltype(ch)>>(
            [&called] {
                called = 1;
            },
            [&called] {
                called = -1;
            })();
        REQUIRE(called == -1);
    }
    {  //  simple true
        auto called = false;
        call_if_constexpr<std::true_type::value>([&called] {
            called = true;
        });
        REQUIRE(called);
    }
    {  //  simple false
        auto called = false;
        call_if_constexpr<std::false_type::value>([&called] {
            called = true;
        });
        REQUIRE(!called);
    }
}
