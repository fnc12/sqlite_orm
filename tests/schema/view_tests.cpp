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

    auto table = make_table("user",
                            make_column("id", &User::id, primary_key().autoincrement()),
                            make_column("name", &User::name));
    auto view = make_view<UserViewSchemaTests>("user_view", select(columns(&User::id, &User::name)));

    SECTION("fields") {
        REQUIRE((view.find_column_name(&UserViewSchemaTests::id) &&
                 *view.find_column_name(&UserViewSchemaTests::id) == "id"));
        REQUIRE((view.find_column_name(&UserViewSchemaTests::name) &&
                 *view.find_column_name(&UserViewSchemaTests::name) == "name"));
    }
}
#endif
