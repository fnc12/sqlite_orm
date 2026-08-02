#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_WITH_VIEW
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
using namespace sqlite_orm;

namespace {
    struct[[= "user_view"_orm_name]] UserViewSchemaTests {
        int64 id = 0;
        std::string name;
    };

    struct UserViewSchemaTestsDefaultName {
        int64 id = 0;
        std::string name;
    };
}

TEST_CASE("view make_view name resolution") {
    struct User {
        int64 id = 0;
        std::string name;
    };

    SECTION("annotation supplies the view name") {
        auto view = make_view<UserViewSchemaTests>(select(asterisk<User>()));
        REQUIRE(view.name == "user_view");
    }
    SECTION("fallback to type identifier") {
        auto view = make_view<UserViewSchemaTestsDefaultName>(select(asterisk<User>()));
        REQUIRE(view.name == "UserViewSchemaTestsDefaultName");
    }
}

TEST_CASE("view::find_column_name") {
    struct User {
        int64 id = 0;
        std::string name;
    };

    SECTION("fields, direct") {
        auto view = make_view<UserViewSchemaTests>(select(asterisk<User>()));

        REQUIRE((view.find_column_name(&UserViewSchemaTests::id) &&
                 *view.find_column_name(&UserViewSchemaTests::id) == "id"));
        REQUIRE((view.find_column_name(&UserViewSchemaTests::name) &&
                 *view.find_column_name(&UserViewSchemaTests::name) == "name"));
    }
    SECTION("fields, derived") {
        struct DerivedUserView : UserViewSchemaTests {};
        auto view = make_view<DerivedUserView>(select(asterisk<User>()));

        REQUIRE((view.find_column_name(&UserViewSchemaTests::id) &&
                 *view.find_column_name(&UserViewSchemaTests::id) == "id"));
        REQUIRE((view.find_column_name(&UserViewSchemaTests::name) &&
                 *view.find_column_name(&UserViewSchemaTests::name) == "name"));
    }
}
#endif
#endif
