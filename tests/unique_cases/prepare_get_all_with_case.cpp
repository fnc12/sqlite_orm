#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Prepare with case") {
    struct UserProfile {
        int id = 0;
        std::string firstName;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        UserProfile() = default;
        UserProfile(int id, std::string firstName) : id{id}, firstName{move(firstName)} {}
#endif
    };

    auto storage = make_storage({},
                                make_table("user_profiles",
                                           make_column("id", &UserProfile::id, primary_key()),
                                           make_column("first_name", &UserProfile::firstName)));
    storage.sync_schema();

    const std::string name = "Iggy";

    SECTION("case string name") {
        auto statement = storage.prepare(get_all<UserProfile>(where(is_equal(
            &UserProfile::firstName,
            case_<std::string>(name).when((length(name) > 0), then(name)).else_(&UserProfile::firstName).end()))));
        std::ignore = statement;
    }
    SECTION("case string") {
        auto statement = storage.prepare(get_all<UserProfile>(where(
            case_<std::string>().when(length(name) > 0, then(like(&UserProfile::firstName, name))).else_(true).end())));
        std::ignore = statement;
    }
    SECTION("case int") {
        storage.insert(UserProfile{1, "Iggy"});
        storage.insert(UserProfile{2, "I%"});
        auto rows = storage.get_all<UserProfile>();
        auto statement3 = storage.prepare(get_all<UserProfile>(
            where(is_equal(&UserProfile::id,
                           case_<int>(c(&UserProfile::id) * 2).when(2, then(1)).when(4, then(2)).else_(3).end()))));
        auto r2 = storage.execute(statement3);
        REQUIRE(r2.size() == rows.size());  // does not filter at all
    }
}
