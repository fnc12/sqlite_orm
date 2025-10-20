#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::alias_extractor;

TEST_CASE("table name collector") {
    struct User {
        int id = 0;
        std::string name;
    };

    const std::tuple dbObjects{make_table("users", make_column("id", &User::id), make_column("name", &User::name))};

    internal::table_name_collector_base::table_name_set expected;

    SECTION("static tests") {
        STATIC_REQUIRE(polyfill::is_invocable<internal::table_name_collector<std::tuple<>>,
                                              std::true_type,
                                              const internal::highlight_t<User, int, int, int>&>::value);
    }
    SECTION("from table") {
        const std::string& tableName = std::get<0>(dbObjects).name;
        internal::table_name_collector collector(dbObjects);

        SECTION("regular column") {
            auto expression = &User::id;
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        SECTION("regular column pointer") {
            auto expression = column<User>(&User::id);
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        SECTION("aliased regular column") {
            using als = alias_z<User>;
            auto expression = alias_column<als>(&User::id);
            expected.emplace(tableName, "z");
            iterate_ast(expression, collector);
        }
        SECTION("aliased regular column pointer") {
            using als = alias_z<User>;
            auto expression = alias_column<als>(column<User>(&User::id));
            expected.emplace(tableName, "z");
            iterate_ast(expression, collector);
        }
        SECTION("count asterisk") {
            auto expression = count<User>();
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        REQUIRE(collector.table_names == expected);
    }
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
    SECTION("from hidden") {
        const std::tuple dbObjects2{make_dbstat_table()};
        const std::string& tableName = std::get<0>(dbObjects2).name;
        internal::table_name_collector collector(dbObjects2);

        SECTION("regular column") {
            auto expression = &dbstat::hidden::schema;
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        SECTION("regular column pointer") {
            auto expression = dbstat_table->*&dbstat::hidden::schema;
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        REQUIRE(collector.table_names == expected);
    }
#endif
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    SECTION("from CTE") {
        const auto dbObjects2 =
            internal::db_objects_cat(dbObjects, internal::make_cte_db_object(dbObjects, 1_ctealias().as(select(1))));
        const std::string& tableName = std::get<0>(dbObjects2).name;
        internal::table_name_collector collector(dbObjects2);

        SECTION("CTE column") {
            using cte_1 = decltype(1_ctealias);
            auto expression = column<cte_1>(&User::id);
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        SECTION("CTE column alias") {
            using cte_1 = decltype(1_ctealias);
            auto expression = column<cte_1>(1_colalias);
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        SECTION("CTE count asterisk") {
            using cte_1 = decltype(1_ctealias);
            auto expression = count<cte_1>();
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        SECTION("aliased CTE column") {
            constexpr auto c = "1"_cte;
            constexpr auto z_alias = "z"_alias.for_<c>();
            auto expression = z_alias->*&User::id;
            expected.emplace(tableName, "z");
            iterate_ast(expression, collector);
        }
        SECTION("aliased CTE column alias") {
            constexpr auto c = "1"_cte;
            constexpr auto z_alias = "z"_alias.for_<c>();
            auto expression = z_alias->*1_colalias;
            expected.emplace(tableName, "z");
            iterate_ast(expression, collector);
        }
        SECTION("CTE count asterisk 2") {
            constexpr auto c = 1_ctealias;
            auto expression = count<c>();
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
#endif
        REQUIRE(collector.table_names == expected);
    }
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    SECTION("highlight") {
        using user_hidden = fts5::hidden_fields_of<User>;
        const std::string& tableName = std::get<0>(dbObjects).name;
        internal::table_name_collector collector(dbObjects);

        SECTION("simple") {
            auto expression = highlight(user_hidden::any_field, 0, "<b>", "</b>");
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        SECTION("in columns") {
            auto expression = columns(highlight(user_hidden::any_field, 0, "<b>", "</b>"));
            expected.emplace(tableName, "");
            iterate_ast(expression, collector);
        }
        REQUIRE(collector.table_names == expected);
    }
#endif
}
