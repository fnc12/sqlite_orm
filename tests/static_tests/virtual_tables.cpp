#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::col_index_sequence_of, internal::col_index_sequence_with_field_type;
using internal::is_base_template_of_v;
using internal::table_definition;

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("dbstat") {
    SECTION("table definition") {
        auto table = make_dbstat_table();
        using table_type = decltype(table);
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(std::is_same<table_type::module_type, internal::dbstat_module_tag>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type, dbstat>::value);
        STATIC_REQUIRE(is_base_template_of_v<table_definition, table_type>);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 10);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::string>::size() == 3);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 7);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        STATIC_REQUIRE(orm_table_reference<decltype(dbstat_table)>);
#endif
    }
}
#endif
