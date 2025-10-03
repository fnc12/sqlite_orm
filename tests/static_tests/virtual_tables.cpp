#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::col_index_sequence_of, internal::col_index_sequence_with, internal::col_index_sequence_with_field_type;
using internal::is_base_template_of_v;
using internal::table_definition, internal::insertable_table_definition;

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("generic vtab and dbstat static tests") {
    using internal::dbstat_module_tag;
    SECTION("table definition") {
        auto table = make_dbstat_table();
        using table_type = decltype(table);
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(std::is_same<table_type::module_type, dbstat_module_tag>::value);
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

#ifdef SQLITE_ENABLE_RTREE
TEST_CASE("rtree static tests") {
    using internal::rtree_module_tag;
    struct DemoIndex {
        int64 id;
        float minX, maxX;
        float minY, maxY;
        float minZ, maxZ;
        float minA, maxA;
        float minB, maxB;

        std::string objname;
        std::string objtype;
    };

    SECTION("1 dimension definition") {
        auto definition = using_rtree(make_column("id", &DemoIndex::id, primary_key()),
                                      make_column("minX", &DemoIndex::minX),
                                      make_column("maxX", &DemoIndex::maxX));
        using definition_type = decltype(definition);
        using elements_type = decltype(definition.elements);
        STATIC_REQUIRE(std::is_same<definition_type::module_type, rtree_module_tag>::value);
        STATIC_REQUIRE(is_base_template_of_v<insertable_table_definition, definition_type>);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 3);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, float>::size() == 2);
    }
    SECTION("5 dimensions definition") {
        auto definition = using_rtree(make_column("id", &DemoIndex::id, primary_key()),
                                      make_column("minX", &DemoIndex::minX),
                                      make_column("maxX", &DemoIndex::maxX),
                                      make_column("minY", &DemoIndex::minY),
                                      make_column("maxY", &DemoIndex::maxY),
                                      make_column("minZ", &DemoIndex::minZ),
                                      make_column("maxZ", &DemoIndex::maxZ),
                                      make_column("minA", &DemoIndex::minA),
                                      make_column("maxA", &DemoIndex::maxA),
                                      make_column("minB", &DemoIndex::minB),
                                      make_column("maxB", &DemoIndex::maxB));
        using definition_type = decltype(definition);
        using elements_type = decltype(definition.elements);
        STATIC_REQUIRE(std::is_same<definition_type::module_type, rtree_module_tag>::value);
        STATIC_REQUIRE(is_base_template_of_v<insertable_table_definition, definition_type>);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 11);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, float>::size() == 10);
    }
#if SQLITE_VERSION_NUMBER >= 3024000
    SECTION("auxiliary columns definition") {
        auto definition = using_rtree(make_column("id", &DemoIndex::id, primary_key()),
                                      make_column("minX", &DemoIndex::minX),
                                      make_column("maxX", &DemoIndex::maxX),
                                      make_column("objname", &DemoIndex::objname, auxiliary()),
                                      make_column("objtype", &DemoIndex::objtype, auxiliary()));
        using definition_type = decltype(definition);
        using elements_type = decltype(definition.elements);
        STATIC_REQUIRE(std::is_same<definition_type::module_type, rtree_module_tag>::value);
        STATIC_REQUIRE(is_base_template_of_v<insertable_table_definition, definition_type>);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 5);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, float>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::string>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, internal::is_auxiliary>::size() == 2);
    }
#endif
}

TEST_CASE("rtree_i32 static tests") {
    SECTION("table definition") {
        struct DemoIndex {
            int64 id;
            std::int32_t minX, maxX;
        };

        auto definition = using_rtree_i32(make_column("id", &DemoIndex::id, primary_key()),
                                          make_column("minX", &DemoIndex::minX),
                                          make_column("maxX", &DemoIndex::maxX));
        using definition_type = decltype(definition);
        using elements_type = decltype(definition.elements);
        STATIC_REQUIRE(std::is_same<definition_type::module_type, internal::rtree_i32_module_tag>::value);
        STATIC_REQUIRE(is_base_template_of_v<insertable_table_definition, definition_type>);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 3);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::int32_t>::size() == 2);
    }
}
#endif
