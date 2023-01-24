#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same
#include <cstddef>  //  nullptr_t

using namespace sqlite_orm;
using std::is_same;
using std::nullptr_t;
#if __cpp_lib_type_trait_variable_templates >= 201510L
using std::is_same_v;
#endif

TEST_CASE("Builtin function return types") {
    struct User {
        int64 id;
        bool flag;

        bool getFlag() const {
            return this->flag;
        }
    };

    STATIC_REQUIRE(is_same<decltype(coalesce(&User::id, 0))::return_type, int64>::value);
    STATIC_REQUIRE(is_same<decltype(coalesce(&User::flag, false))::return_type, bool>::value);
    STATIC_REQUIRE(is_same<decltype(coalesce(&User::getFlag, false))::return_type, bool>::value);
    STATIC_REQUIRE(is_same<decltype(coalesce(&User::flag, 0))::return_type, int>::value);
    STATIC_REQUIRE(is_same<decltype(coalesce<int>(&User::flag, false))::return_type, int>::value);
    // note: return type nullptr_t doesn't make sense but works for unit tests to assert intention
    STATIC_REQUIRE(is_same<decltype(coalesce<nullptr_t>(&User::flag, nullptr))::return_type, nullptr_t>::value);

    STATIC_REQUIRE(is_same<decltype(ifnull(&User::id, 0))::return_type, int64>::value);
    STATIC_REQUIRE(is_same<decltype(ifnull(&User::flag, false))::return_type, bool>::value);
    STATIC_REQUIRE(is_same<decltype(ifnull(&User::getFlag, false))::return_type, bool>::value);
    STATIC_REQUIRE(is_same<decltype(ifnull(&User::flag, 0))::return_type, int>::value);
    STATIC_REQUIRE(is_same<decltype(ifnull<int>(&User::flag, false))::return_type, int>::value);
    // note: return type nullptr_t doesn't make sense but works for unit tests to assert intention
    STATIC_REQUIRE(is_same<decltype(coalesce<nullptr_t>(&User::flag, nullptr))::return_type, nullptr_t>::value);

#if defined(SQLITE_ORM_OPTIONAL_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
    STATIC_REQUIRE(is_same_v<decltype(nullif(&User::id, 0))::return_type, std::optional<int64>>);
    STATIC_REQUIRE(is_same_v<decltype(nullif(&User::flag, false))::return_type, std::optional<bool>>);
    STATIC_REQUIRE(is_same_v<decltype(nullif(&User::getFlag, false))::return_type, std::optional<bool>>);
    STATIC_REQUIRE(is_same_v<decltype(nullif(&User::flag, 0))::return_type, std::optional<int>>);
#endif
    // note: return type nullptr_t doesn't make sense but works for unit tests to assert intention
    {
        STATIC_REQUIRE(is_same<decltype(nullif<nullptr_t>(&User::id, 0))::return_type, nullptr_t>::value);
        STATIC_REQUIRE(is_same<decltype(nullif<nullptr_t>(&User::flag, false))::return_type, nullptr_t>::value);
        STATIC_REQUIRE(is_same<decltype(nullif<nullptr_t>(&User::getFlag, false))::return_type, nullptr_t>::value);
        STATIC_REQUIRE(is_same<decltype(nullif<nullptr_t>(&User::flag, 0))::return_type, nullptr_t>::value);
    }
}
