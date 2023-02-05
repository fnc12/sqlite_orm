#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::alias_extractor;

TEST_CASE("table name collector") {
    struct User {
        int id = 0;
        std::string name;
    };

    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    internal::table_name_collector::table_name_set expected;

    SECTION("from table") {
        internal::serializer_context<db_objects_t> context{dbObjects};
        internal::table_name_collector collector([&context](const std::type_index& ti) {
            return internal::find_table_name(context.db_objects, ti);
        });

        SECTION("regular column") {
            using als = alias_z<User>;
            auto expression = &User::id;
            expected.emplace(table.name, "");
            iterate_ast(expression, collector);
        }
        SECTION("regular column pointer") {
            auto expression = column<User>(&User::id);
            expected.emplace(table.name, "");
            iterate_ast(expression, collector);
        }
        SECTION("aliased regular column") {
            using als = alias_z<User>;
            auto expression = alias_column<als>(&User::id);
            expected.emplace(table.name, "z");
            iterate_ast(expression, collector);
        }
        SECTION("aliased regular column pointer") {
            using als = alias_z<User>;
            auto expression = alias_column<als>(column<User>(&User::id));
            expected.emplace(table.name, "z");
            iterate_ast(expression, collector);
        }
        REQUIRE(collector.table_names == expected);
    }
}
