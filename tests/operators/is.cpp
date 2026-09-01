#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <optional>  //  std::optional

using namespace sqlite_orm;

TEST_CASE("is operators") {
    struct User {
        int id = 0;
        std::optional<std::string> middleName;
    };
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("middle_name", &User::middleName)));
    storage.sync_schema();
    storage.replace(User{1, std::nullopt});
    storage.replace(User{2, "Eldarovich"});

    std::vector<int> rows;
    decltype(rows) expected;
    SECTION("IS matches NULL") {
        rows = storage.select(&User::id, where(is(&User::middleName, std::optional<std::string>{})));
        expected = {1};
    }
    SECTION("IS NOT skips NULL") {
        rows = storage.select(&User::id, where(is_not(&User::middleName, std::optional<std::string>{})));
        expected = {2};
    }
    SECTION("a negated IS behaves like IS NOT") {
        rows = storage.select(&User::id, where(not is(&User::middleName, std::optional<std::string>{})));
        expected = {2};
    }
#if SQLITE_VERSION_NUMBER >= 3039000
    SECTION("IS DISTINCT FROM treats NULLs as equal") {
        rows = storage.select(&User::id, where(is_distinct_from(&User::middleName, std::optional<std::string>{})));
        expected = {2};
    }
    SECTION("IS NOT DISTINCT FROM matches NULL") {
        rows = storage.select(&User::id, where(is_not_distinct_from(&User::middleName, std::optional<std::string>{})));
        expected = {1};
    }
#endif
    REQUIRE(rows == expected);
}
