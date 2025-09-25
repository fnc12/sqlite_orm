#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_WITH_VIEW
using namespace sqlite_orm;

struct UserViewSchemaTests {
    int id = 0;
    std::string name;
};

TEST_CASE("view::find_column_name") {
    struct User {
        int id = 0;
        std::string name;
    };

    auto view = make_view<UserViewSchemaTests>("user_view", select(asterisk<User>()));

    SECTION("fields") {
        REQUIRE((view.find_column_name(&UserViewSchemaTests::id) &&
                 *view.find_column_name(&UserViewSchemaTests::id) == "id"));
        REQUIRE((view.find_column_name(&UserViewSchemaTests::name) &&
                 *view.find_column_name(&UserViewSchemaTests::name) == "name"));
    }
}
#endif
