cmake_minimum_required (VERSION 3.2)

option(SQLITE_ORM_OMITS_CODECVT "Omits codec testing" OFF)

option(SqliteOrm_SysSqlite "Use system version of sqlite library" OFF)

if(SqliteOrm_SysSqlite)
    message(FATAL_ERROR "WIP: please, disable the SqliteOrm_SysSqlite option.")
else()
    add_subdirectory(third_party/sqlite)
endif()

add_executable(unit_tests tests.cpp tests2.cpp tests3.cpp tests4.cpp tests4.cpp private_getters_tests.cpp pragma_tests.cpp explicit_columns.cpp core_functions_tests.cpp composite_key.cpp static_tests.cpp operators/arithmetic_operators.cpp operators/like.cpp operators/glob.cpp operators/in.cpp operators/cast.cpp operators/is_null.cpp dynamic_order_by.cpp prepared_statement_tests/select.cpp prepared_statement_tests/get_all.cpp prepared_statement_tests/get_all_pointer.cpp prepared_statement_tests/get_all_optional.cpp prepared_statement_tests/update_all.cpp prepared_statement_tests/remove_all.cpp prepared_statement_tests/get.cpp prepared_statement_tests/get_pointer.cpp prepared_statement_tests/get_optional.cpp prepared_statement_tests/update.cpp prepared_statement_tests/remove.cpp prepared_statement_tests/insert.cpp prepared_statement_tests/replace.cpp prepared_statement_tests/insert_range.cpp prepared_statement_tests/replace_range.cpp prepared_statement_tests/insert_explicit.cpp pragma_tests.cpp simple_query.cpp static_tests/is_bindable.cpp static_tests/arithmetic_operators_result_type.cpp static_tests/tuple_conc.cpp static_tests/node_tuple.cpp static_tests/bindable_filter.cpp static_tests/count_tuple.cpp constraints/default.cpp constraints/foreign_key.cpp)


if(SQLITE_ORM_OMITS_CODECVT)
	message(STATUS "SQLITE_ORM_OMITS_CODECVT is enabled")
	target_compile_definitions(unit_tests PRIVATE SQLITE_ORM_OMITS_CODECVT=1)
endif()

target_compile_options(unit_tests PRIVATE -Wall -pedantic -Werror)

find_package(Catch2 REQUIRED)
target_link_libraries(unit_tests PRIVATE sqlite_orm sqlite3 Catch2::Catch2)

enable_testing()

add_test(NAME "All_in_one_unit_test"
    COMMAND unit_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
