#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED

using namespace std::literals;

TEST_CASE("column_name_method") {

    struct UserIdAlias : alias_tag {
        static const std::string& get() {
            static const std::string res = "USER_ID";
            return res;
        }
    };

    struct User {
        int id = 0;
        std::string name;
    };
    auto storage = make_storage("column_name.sqlite",
                                make_table("users", make_column("id", &User::id), make_column("name", &User::name)));

    storage.sync_schema();

    auto statement = storage.prepare(select(columns(as<UserIdAlias>(&User::id), &User::name)));
    std::string id_header{statement.column_name(0)};
    std::string_view name_header = statement.column_name(1);

    REQUIRE(id_header == "USER_ID"s);
    REQUIRE(name_header == "name"s);
}

#endif
