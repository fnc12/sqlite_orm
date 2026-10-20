#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using Catch::Matchers::Equals;

TEST_CASE("builtin tables") {
    SECTION("sqlite_schema") {
        auto storage = make_storage("", make_sqlite_schema_table());
        storage.sync_schema();

        auto masterRows = storage.get_all<sqlite_master>();
        std::ignore = masterRows;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        auto schemaRows = storage.get_all<sqlite_schema>();

        STATIC_REQUIRE(std::is_same_v<decltype(masterRows), decltype(schemaRows)>);
        REQUIRE_THAT(schemaRows, Equals(masterRows));

        auto schemaRows2 = storage.get_all<sqlite_master_table>();

        STATIC_REQUIRE(std::is_same_v<decltype(masterRows), decltype(schemaRows2)>);
        REQUIRE_THAT(schemaRows2, Equals(masterRows));

#if __cpp_lib_containers_ranges >= 202202L
        std::vector<sqlite_master> schemaRows3{std::from_range, storage.iterate<sqlite_master_table>()};
#else
        auto view = storage.iterate<sqlite_master_table>();
        std::vector<sqlite_master> schemaRows3{view.begin(), view.end()};
#endif
        REQUIRE_THAT(schemaRows2, Equals(masterRows));
#endif
    }
}

TEST_CASE("builtin tables bundle") {
    using schema_table_type = decltype(make_sqlite_schema_table());
    using bundle_elements_type = decltype(make_builtin_tables());
    constexpr size_t bundleSize = std::tuple_size<bundle_elements_type>::value;

    SECTION("contents") {
        constexpr size_t expectedSize = 1
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
                                        + 1
#endif
#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ORM_REFLECTION_SUPPORTED)
                                        + std::tuple_size<decltype(make_pragma_tables())>::value
#endif
            ;
        STATIC_REQUIRE(bundleSize == expectedSize);
        STATIC_REQUIRE(std::is_same<std::tuple_element_t<0, bundle_elements_type>, schema_table_type>::value);
        STATIC_REQUIRE(internal::is_tuple_of_database_objects<bundle_elements_type>::value);
        STATIC_REQUIRE(internal::is_tuple_of_database_objects<std::tuple<>>::value);
        STATIC_REQUIRE_FALSE(internal::is_tuple_of_database_objects<std::tuple<schema_table_type, int>>::value);
        STATIC_REQUIRE_FALSE(internal::is_tuple_of_database_objects<schema_table_type>::value);
        STATIC_REQUIRE(internal::is_storage_spec<bundle_elements_type>::value);
        STATIC_REQUIRE(internal::is_storage_spec<schema_table_type>::value);
        STATIC_REQUIRE_FALSE(internal::is_storage_spec<int>::value);
    }
    SECTION("a separately passed table replaces the bundled one") {
        auto storage = make_storage("",
                                    make_builtin_tables(),
                                    make_table("sqlite_temp_master",
                                               make_column("type", &sqlite_master::type),
                                               make_column("name", &sqlite_master::name),
                                               make_column("tbl_name", &sqlite_master::tbl_name),
                                               make_column("rootpage", &sqlite_master::rootpage),
                                               make_column("sql", &sqlite_master::sql)));
        using db_objects_type = decltype(storage)::db_objects_type;
        STATIC_REQUIRE(std::tuple_size<db_objects_type>::value == bundleSize);
        REQUIRE(storage.tablename<sqlite_master>() == "sqlite_temp_master");
    }
    SECTION("a table bundled twice is kept once") {
        auto storage = make_storage("",
                                    make_builtin_tables(),
#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ORM_REFLECTION_SUPPORTED)
                                    make_pragma_tables(),
#endif
                                    make_builtin_tables());
        using db_objects_type = decltype(storage)::db_objects_type;
        STATIC_REQUIRE(std::is_same<db_objects_type, bundle_elements_type>::value);
        REQUIRE(storage.tablename<sqlite_master>() == "sqlite_master");
    }
    SECTION("a table twice in a tuple is kept once") {
        auto storage = make_storage("", std::tuple{make_sqlite_schema_table(), make_sqlite_schema_table()});
        using db_objects_type = decltype(storage)::db_objects_type;
        STATIC_REQUIRE(std::is_same<db_objects_type, std::tuple<schema_table_type>>::value);
    }
    SECTION("storage") {
        struct User {
            int64 id = 0;
            std::string name;
        };
        auto storage = make_storage(
            "",
            make_builtin_tables(),
            make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
        storage.sync_schema();

        auto tableNames = storage.select(&sqlite_master::name, where(c(&sqlite_master::type) == "table"));
        REQUIRE_THAT(tableNames, Equals(std::vector<std::string>{"users"}));
#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ORM_REFLECTION_SUPPORTED)
        auto columnNames = storage.select(&table_info::name, from(pragma_table_info("users")));
        REQUIRE_THAT(columnNames, Equals(std::vector<std::string>{"id", "name"}));
#endif
    }
}
