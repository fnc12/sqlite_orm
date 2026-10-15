#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::col_index_sequence_of, internal::col_index_sequence_with, internal::col_index_sequence_with_field_type,
    internal::hidden_col_index_sequence_of, internal::all_col_index_sequence_with_field_type;
using internal::is_base_template_of_v, internal::is_table_reference_v;
using internal::table_definition, internal::insertable_table_definition;

template<class Elements>
using table_values_index_sequence = internal::filter_tuple_sequence_t<Elements, internal::is_literal>;

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("generic vtab and dbstat layout tests") {
    using internal::dbstat_module_tag;
    struct mystat : dbstat {};

    STATIC_REQUIRE(is_table_reference_v<decltype(dbstat_table)>);
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    STATIC_REQUIRE(orm_table_reference<decltype(dbstat_table)>);
#endif

    SECTION("default table definition") {
        auto table = make_dbstat_table();
        using table_type = decltype(table);
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(std::is_same<table_type::module_type, dbstat_module_tag>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type, dbstat>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type::hidden::enclosing_type, dbstat>::value);
        STATIC_REQUIRE(is_base_template_of_v<table_definition, table_type>);
        STATIC_REQUIRE_FALSE(is_base_template_of_v<insertable_table_definition, table_type>);
        STATIC_REQUIRE(table_type::module_traits_type::is_eponymous::value);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 10);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::string>::size() == 3);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 7);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::string>::size() == 3);
        STATIC_REQUIRE(all_col_index_sequence_with_field_type<elements_type, std::string>::size() == 4);
#if SQLITE_VERSION_NUMBER >= 3031000
        STATIC_REQUIRE(all_col_index_sequence_with_field_type<elements_type, bool>::size() == 1);
        STATIC_REQUIRE(hidden_col_index_sequence_of<elements_type>::size() == 2);
#else
        STATIC_REQUIRE(hidden_col_index_sequence_of<elements_type>::size() == 1);
#endif
    }
    SECTION("table definition with table value") {
        auto table = make_virtual_table<mystat>("dbstat",
                                                using_dbstat("main"
#if SQLITE_VERSION_NUMBER >= 3031000
                                                             ,
                                                             true
#endif
                                                             ));
        using elements_type = decltype(table.elements);
#if SQLITE_VERSION_NUMBER >= 3031000
        STATIC_REQUIRE(table_values_index_sequence<elements_type>::size() == 2);
#else
        STATIC_REQUIRE(table_values_index_sequence<elements_type>::size() == 1);
#endif
    }
}
#endif

#if SQLITE_VERSION_NUMBER >= 3008012
TEST_CASE("generate_series layout tests") {
    using internal::generate_series_module_tag;

    STATIC_REQUIRE(is_table_reference_v<decltype(generate_series_table)>);
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    STATIC_REQUIRE(orm_table_reference<decltype(generate_series_table)>);
#endif

    SECTION("default table definition") {
        auto table = make_generate_series_table();
        using table_type = decltype(table);
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(std::is_same<table_type::module_type, generate_series_module_tag>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type, generate_series>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type::hidden::enclosing_type, generate_series>::value);
        STATIC_REQUIRE(is_base_template_of_v<table_definition, table_type>);
        STATIC_REQUIRE_FALSE(is_base_template_of_v<insertable_table_definition, table_type>);
        STATIC_REQUIRE(table_type::module_traits_type::is_eponymous::value);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(all_col_index_sequence_with_field_type<elements_type, int>::size() == 4);
        STATIC_REQUIRE(hidden_col_index_sequence_of<elements_type>::size() == 3);
    }
}
#endif

#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED)
TEST_CASE("pragma_table_info layout tests") {
    using internal::pragma_table_info_module_tag;

    STATIC_REQUIRE(is_table_reference_v<decltype(pragma_table_info_table)>);
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    STATIC_REQUIRE(orm_table_reference<decltype(pragma_table_info_table)>);
#endif

    SECTION("default table definition") {
        auto table = make_pragma_table_info_table();
        using table_type = decltype(table);
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(std::is_same<table_type::module_type, pragma_table_info_module_tag>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type, table_info>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type::hidden::enclosing_type, table_info>::value);
        STATIC_REQUIRE(is_base_template_of_v<table_definition, table_type>);
        STATIC_REQUIRE_FALSE(is_base_template_of_v<insertable_table_definition, table_type>);
        STATIC_REQUIRE(table_type::module_traits_type::is_eponymous::value);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 6);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, bool>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::string>::size() == 3);
        STATIC_REQUIRE(all_col_index_sequence_with_field_type<elements_type, std::string>::size() == 5);
        STATIC_REQUIRE(hidden_col_index_sequence_of<elements_type>::size() == 2);
    }
}

#if SQLITE_VERSION_NUMBER >= 3026000
TEST_CASE("pragma_table_xinfo layout tests") {
    using internal::pragma_table_xinfo_module_tag;

    STATIC_REQUIRE(is_table_reference_v<decltype(pragma_table_xinfo_table)>);
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    STATIC_REQUIRE(orm_table_reference<decltype(pragma_table_xinfo_table)>);
#endif

    SECTION("data member and nested struct named `hidden`") {
        STATIC_REQUIRE(std::is_same<decltype(&table_xinfo::hidden), int table_xinfo::*>::value);
        STATIC_REQUIRE(std::is_same<decltype(&table_xinfo::hidden::arg), std::string table_xinfo::hidden::*>::value);
        STATIC_REQUIRE(std::is_same<internal::mapped_type_of_t<struct table_xinfo::hidden>, table_xinfo>::value);
    }
    SECTION("default table definition") {
        auto table = make_pragma_table_xinfo_table();
        using table_type = decltype(table);
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(std::is_same<table_type::module_type, pragma_table_xinfo_module_tag>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type, table_xinfo>::value);
        STATIC_REQUIRE(std::is_same<table_type::object_type::hidden::enclosing_type, table_xinfo>::value);
        STATIC_REQUIRE(is_base_template_of_v<table_definition, table_type>);
        STATIC_REQUIRE_FALSE(is_base_template_of_v<insertable_table_definition, table_type>);
        STATIC_REQUIRE(table_type::module_traits_type::is_eponymous::value);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 7);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 3);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, bool>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::string>::size() == 3);
        STATIC_REQUIRE(all_col_index_sequence_with_field_type<elements_type, std::string>::size() == 5);
        STATIC_REQUIRE(hidden_col_index_sequence_of<elements_type>::size() == 2);
    }
}
#endif
#endif

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
TEST_CASE("fts5 layout tests") {
    using internal::fts5_module_tag;
    struct Post {
        std::string title;
        std::string body;
    };

    STATIC_REQUIRE_FALSE(std::is_constructible<fts5>::value);

    SECTION("table definition") {
        auto definition = using_fts5(make_column("title", &Post::title), make_column("body", &Post::body));
        using definition_type = decltype(definition);
        using elements_type = decltype(definition.elements);
        STATIC_REQUIRE(std::is_same<definition_type::module_type, fts5_module_tag>::value);
        STATIC_REQUIRE(is_base_template_of_v<insertable_table_definition, definition_type>);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::string>::size() == 2);
        STATIC_REQUIRE(hidden_col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(all_col_index_sequence_with_field_type<elements_type, std::vector<char>>::size() == 1);
        STATIC_REQUIRE(all_col_index_sequence_with_field_type<elements_type, std::optional<int>>::size() == 1);
    }
}
#endif

#ifdef SQLITE_ENABLE_RTREE
TEST_CASE("rtree layout tests") {
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
        STATIC_REQUIRE(hidden_col_index_sequence_of<elements_type>::size() == 0);
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
    using internal::rtree_i32_module_tag;
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
        STATIC_REQUIRE(std::is_same<definition_type::module_type, rtree_i32_module_tag>::value);
        STATIC_REQUIRE(is_base_template_of_v<insertable_table_definition, definition_type>);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 3);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, std::int32_t>::size() == 2);
    }
}
#endif
