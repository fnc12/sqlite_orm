#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Prepare with case") {
    struct UserProfile {
        int id = 0;
        std::string firstName;
    };

    auto storage = make_storage({},
                                make_table("user_profiles",
                                           make_column("id", &UserProfile::id, primary_key()),
                                           make_column("first_name", &UserProfile::firstName)));
    storage.sync_schema();

    const std::string name = "Iggy";

    {
        auto statement = storage.prepare(get_all<UserProfile>(where(is_equal(
            &UserProfile::firstName,
            case_<std::string>(name).when((length(name) > 0), then(name)).else_(&UserProfile::firstName).end()))));
        std::ignore = statement;
    }
    {
        auto statement = storage.prepare(get_all<UserProfile>(where(
            case_<std::string>().when(length(name) > 0, then(like(&UserProfile::firstName, name))).else_(true).end())));
        std::ignore = statement;
    }
}
