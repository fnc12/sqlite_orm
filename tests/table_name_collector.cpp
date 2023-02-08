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
    internal::table_name_collector_base::table_name_set expected;

    SECTION("from table") {
        internal::serializer_context<db_objects_t> context{dbObjects};
        auto collector = internal::make_table_name_collector(context.db_objects);

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

#ifdef SQLITE_ORM_WITH_CTE
    SECTION("from CTE") {
        auto dbObjects2 =
            internal::storage_db_objects_cat(dbObjects, internal::make_cte_table(dbObjects, cte<cte_1>()(select(1))));
        using context_t = internal::serializer_context<decltype(dbObjects2)>;
        context_t context{dbObjects2};
        auto collector = internal::make_table_name_collector(context.db_objects);

        SECTION("CTE column") {
            constexpr auto c = 1_ctealias;
            auto expression = c->*&User::id;
            expected.emplace(alias_extractor<decltype(c)>::extract(), "");
            iterate_ast(expression, collector);
        }
        SECTION("CTE column alias") {
            constexpr auto c = 1_ctealias;
            auto expression = c->*1_colalias;
            expected.emplace(alias_extractor<decltype(c)>::extract(), "");
            iterate_ast(expression, collector);
        }
#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
        SECTION("aliased CTE column") {
            constexpr auto c = "1"_cte;
            constexpr auto z_alias = "z"_alias(c);
            auto expression = z_alias->*&User::id;
            expected.emplace(alias_extractor<decltype(c)>::extract(), "z");
            iterate_ast(expression, collector);
        }
        SECTION("aliased CTE column alias") {
            constexpr auto c = "1"_cte;
            constexpr auto z_alias = "z"_alias(c);
            auto expression = z_alias->*1_colalias;
            expected.emplace(alias_extractor<decltype(c)>::extract(), "z");
            iterate_ast(expression, collector);
        }
#endif
        REQUIRE(collector.table_names == expected);
    }
#endif
}
