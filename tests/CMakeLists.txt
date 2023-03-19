cmake_minimum_required (VERSION 3.16)

# note: find_package(SQLite3 REQUIRED) already done in top-level CMakeLists
FetchContent_MakeAvailable(Catch2)

option(SQLITE_ORM_OMITS_CODECVT "Omits codec testing" OFF)

add_executable(unit_tests
    static_tests/functional/static_if_tests.cpp
    static_tests/functional/mpl.cpp
    static_tests/functional/same_or_void.cpp
    static_tests/functional/tuple_conc.cpp
    static_tests/functional/tuple_filter.cpp
    static_tests/functional/tuple_traits.cpp
    static_tests/functional/tuple_transform.cpp
    static_tests/is_printable.cpp
    static_tests/is_bindable.cpp
    static_tests/iterator_t.cpp
    static_tests/arithmetic_operators_result_type.cpp
    static_tests/node_tuple.cpp
    static_tests/bindable_filter.cpp
    static_tests/member_traits_tests.cpp
    static_tests/select_return_type.cpp
    static_tests/column.cpp
    static_tests/foreign_key.cpp
    static_tests/function_static_tests.cpp
    static_tests/aggregate_function_return_types.cpp
    static_tests/core_function_return_types.cpp
    static_tests/column_result_t.cpp
    static_tests/alias.cpp
    static_tests/is_primary_key_insertable.cpp
    static_tests/is_column_with_insertable_primary_key.cpp
    static_tests/operators_adl.cpp
    tuple_iteration.cpp
    sync_schema_tests.cpp
    tests.cpp
    tests2.cpp
    tests3.cpp
    tests4.cpp
    tests5.cpp
    column_tests.cpp
    private_getters_tests.cpp
    pragma_tests.cpp
    explicit_columns.cpp
    built_in_functions_tests/core_functions_tests.cpp
    built_in_functions_tests/datetime_function_tests.cpp
    built_in_functions_tests/math_functions.cpp
    index_tests.cpp
    constraints/composite_key.cpp
    operators/arithmetic_operators.cpp
    operators/like.cpp
    operators/glob.cpp
    operators/in.cpp
    operators/cast.cpp
    operators/is_null.cpp
    operators/not_operator.cpp
    operators/bitwise.cpp
    operators/binary_operators.cpp
    prepared_statement_tests/select.cpp
    prepared_statement_tests/get_all.cpp
    prepared_statement_tests/get_all_pointer.cpp
    prepared_statement_tests/get_all_optional.cpp
    prepared_statement_tests/update_all.cpp
    prepared_statement_tests/remove_all.cpp
    prepared_statement_tests/get.cpp
    prepared_statement_tests/get_pointer.cpp
    prepared_statement_tests/get_optional.cpp
    prepared_statement_tests/update.cpp
    prepared_statement_tests/remove.cpp
    prepared_statement_tests/insert.cpp
    prepared_statement_tests/replace.cpp
    prepared_statement_tests/insert_range.cpp
    prepared_statement_tests/replace_range.cpp
    prepared_statement_tests/insert_explicit.cpp
    prepared_statement_tests/column_names.cpp
    pragma_tests.cpp
    simple_query.cpp
    constraints/default.cpp
    constraints/unique.cpp
    constraints/foreign_key.cpp
    constraints/check.cpp
    table_tests.cpp
    statement_serializer_tests/column_constraints/generated.cpp
    statement_serializer_tests/column_constraints/default.cpp
    statement_serializer_tests/column_constraints/primary_key.cpp
    statement_serializer_tests/column_constraints/autoincrement.cpp
    statement_serializer_tests/column_constraints/unique.cpp
    statement_serializer_tests/column_constraints/check.cpp
    statement_serializer_tests/bindables.cpp
    statement_serializer_tests/ast/upsert_clause.cpp
    statement_serializer_tests/ast/excluded.cpp
    statement_serializer_tests/ast/set.cpp
    statement_serializer_tests/arithmetic_operators.cpp
    statement_serializer_tests/base_types.cpp
    statement_serializer_tests/collate.cpp
    statement_serializer_tests/comparison_operators.cpp
    statement_serializer_tests/core_functions.cpp
    statement_serializer_tests/rowid.cpp
    statement_serializer_tests/column_names.cpp
    statement_serializer_tests/foreign_key.cpp
    statement_serializer_tests/schema/index.cpp
    statement_serializer_tests/schema/trigger.cpp
    statement_serializer_tests/schema/new_old.cpp
    statement_serializer_tests/schema/raise.cpp
    statement_serializer_tests/indexed_column.cpp
    statement_serializer_tests/logical_operators.cpp
    statement_serializer_tests/statements/select.cpp
    statement_serializer_tests/select_constraints.cpp
    statement_serializer_tests/conditions.cpp
    statement_serializer_tests/statements/insert_replace.cpp
    statement_serializer_tests/statements/update.cpp
    statement_serializer_tests/statements/remove.cpp
    statement_serializer_tests/statements/update_all.cpp
    statement_serializer_tests/aggregate_functions.cpp
    statement_serializer_tests/alias_extractor.cpp
    storage_tests.cpp
    storage_non_crud_tests.cpp
    unique_cases/get_all_with_two_tables.cpp
    unique_cases/prepare_get_all_with_case.cpp
    unique_cases/index_named_table_with_fk.cpp
    unique_cases/issue525.cpp
    unique_cases/delete_with_two_fields.cpp
    unique_cases/join_iterator_ctor_compilation_error.cpp
    unique_cases/issue86.cpp
    unique_cases/issue937.cpp
    unique_cases/issue663.cpp
    get_all_custom_containers.cpp
    select_constraints_tests.cpp
    backup_tests.cpp
    transaction_tests.cpp
    json.cpp
    row_id.cpp
    trigger_tests.cpp
    ast_iterator_tests.cpp
    table_name_collector.cpp
    pointer_passing_interface.cpp
)

if(SQLITE_ORM_OMITS_CODECVT)
    message(STATUS "SQLITE_ORM_OMITS_CODECVT is enabled")
    target_compile_definitions(unit_tests PRIVATE SQLITE_ORM_OMITS_CODECVT=1)
endif()

if (MSVC)
    target_compile_options(unit_tests PUBLIC
        # multi-processor compilation
        /MP)
    if (MSVC_VERSION LESS_EQUAL 1900)
        target_compile_options(unit_tests PUBLIC
            # C4503: decorated name length exceeded
            /wd4503
            # C4800: forcing value to bool (performance warning)
            /wd4800)
    else()
        target_compile_options(unit_tests PUBLIC
            # warning-level 4
            /W4
            # C4127: conditional expression is constant
            /wd4127
            # C4456: declaration of 'symbol' hides previous local declaration
            /wd4456
            # C4458: declaration of 'symbol' hides class member
            /wd4458)
    endif()
endif()

target_precompile_headers(unit_tests PRIVATE
    <sqlite_orm/sqlite_orm.h>
    <catch2/catch_all.hpp>)

# note: sqlite3 already linked in top-level CMakeLists
target_link_libraries(unit_tests PRIVATE sqlite_orm Catch2::Catch2WithMain)

add_test(NAME "All_in_one_unit_test"
    COMMAND unit_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
