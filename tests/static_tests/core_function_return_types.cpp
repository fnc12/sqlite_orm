#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same
#include <cstddef>  //  nullptr_t

using namespace sqlite_orm;
using std::is_same;
using std::is_same_v;
using std::nullptr_t;

// the result type of a built-in function call, as a select statement would yield it
template<class E>
using result_of = internal::column_result_of_t<internal::db_objects_tuple<>, E>;

TEST_CASE("Builtin function return types") {
    struct User {
        int64 id;
        bool flag;

        bool getFlag() const {
            return this->flag;
        }
    };

    STATIC_REQUIRE(is_same<result_of<decltype(coalesce(&User::id, 0))>, int64>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(coalesce(&User::flag, false))>, bool>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(coalesce(&User::getFlag, false))>, bool>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(coalesce(&User::flag, 0))>, int>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(coalesce<int>(&User::flag, false))>, int>::value);
    // note: return type nullptr_t doesn't make sense but works for unit tests to assert intention
    STATIC_REQUIRE(is_same<result_of<decltype(coalesce<nullptr_t>(&User::flag, nullptr))>, nullptr_t>::value);

    STATIC_REQUIRE(is_same<result_of<decltype(ifnull(&User::id, 0))>, int64>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(ifnull(&User::flag, false))>, bool>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(ifnull(&User::getFlag, false))>, bool>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(ifnull(&User::flag, 0))>, int>::value);
    STATIC_REQUIRE(is_same<result_of<decltype(ifnull<int>(&User::flag, false))>, int>::value);
    // note: return type nullptr_t doesn't make sense but works for unit tests to assert intention
    STATIC_REQUIRE(is_same<result_of<decltype(coalesce<nullptr_t>(&User::flag, nullptr))>, nullptr_t>::value);

    STATIC_REQUIRE(is_same_v<result_of<decltype(nullif(&User::id, 0))>, std::optional<int64>>);
    STATIC_REQUIRE(is_same_v<result_of<decltype(nullif(&User::flag, false))>, std::optional<bool>>);
    STATIC_REQUIRE(is_same_v<result_of<decltype(nullif(&User::getFlag, false))>, std::optional<bool>>);
    STATIC_REQUIRE(is_same_v<result_of<decltype(nullif(&User::flag, 0))>, std::optional<int>>);
    // note: return type nullptr_t doesn't make sense but works for unit tests to assert intention
    {
        STATIC_REQUIRE(is_same<result_of<decltype(nullif<nullptr_t>(&User::id, 0))>, nullptr_t>::value);
        STATIC_REQUIRE(is_same<result_of<decltype(nullif<nullptr_t>(&User::flag, false))>, nullptr_t>::value);
        STATIC_REQUIRE(is_same<result_of<decltype(nullif<nullptr_t>(&User::getFlag, false))>, nullptr_t>::value);
        STATIC_REQUIRE(is_same<result_of<decltype(nullif<nullptr_t>(&User::flag, 0))>, nullptr_t>::value);
    }
}
