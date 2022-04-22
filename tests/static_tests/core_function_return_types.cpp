#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("Builtin function return types") {
    struct User {
        int64 id;
        bool flag;

        bool getFlag() const {
            return this->flag;
        }
    };

    STATIC_REQUIRE(std::is_same<decltype(coalesce(&User::id, 0))::return_type, int64>::value);
    STATIC_REQUIRE(std::is_same<decltype(coalesce(&User::flag, false))::return_type, bool>::value);
    STATIC_REQUIRE(std::is_same<decltype(coalesce(&User::getFlag, false))::return_type, bool>::value);
    STATIC_REQUIRE(std::is_same<decltype(coalesce(&User::flag, 0))::return_type, int>::value);

    STATIC_REQUIRE(std::is_same<decltype(ifnull(&User::id, 0))::return_type, int64>::value);
    STATIC_REQUIRE(std::is_same<decltype(ifnull(&User::flag, false))::return_type, bool>::value);
    STATIC_REQUIRE(std::is_same<decltype(ifnull(&User::getFlag, false))::return_type, bool>::value);
    STATIC_REQUIRE(std::is_same<decltype(ifnull(&User::flag, 0))::return_type, int>::value);

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    STATIC_REQUIRE(std::is_same_v<decltype(nullif(&User::id, 0))::return_type, std::optional<int64>>);
    STATIC_REQUIRE(std::is_same_v<decltype(nullif(&User::flag, false))::return_type, std::optional<bool>>);
    STATIC_REQUIRE(std::is_same_v<decltype(nullif(&User::getFlag, false))::return_type, std::optional<bool>>);
    STATIC_REQUIRE(std::is_same_v<decltype(nullif(&User::flag, 0))::return_type, std::optional<int>>);
#endif
}
