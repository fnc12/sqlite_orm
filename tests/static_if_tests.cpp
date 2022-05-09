#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("static_if") {
    {  //  simple true
        auto value = 0;
        internal::static_if<std::true_type::value>(
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
        internal::static_if<std::false_type::value>(
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
        internal::static_if<std::is_empty<std::tuple<>>::value>(
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
        internal::static_if<polyfill::negation_v<std::is_empty<std::tuple<>>>>(
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
        internal::static_if<internal::is_column_v<decltype(ch)>>(
            [&called] {
                called = 1;
            },
            [&called] {
                called = -1;
            })();
        REQUIRE(called == -1);
    }
}
