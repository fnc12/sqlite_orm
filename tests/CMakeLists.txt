cmake_minimum_required (VERSION 3.2)

option(SQLITE_ORM_OMITS_CODECVT "Omits codec testing" OFF)

add_executable(unit_tests
	tests.cpp
	tests2.cpp
	tests3.cpp
	tests4.cpp
	tests5.cpp
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
	pragma_tests.cpp
	simple_query.cpp
	static_tests/is_bindable.cpp
	static_tests/iterator_t.cpp
	static_tests/arithmetic_operators_result_type.cpp
	static_tests/tuple_conc.cpp
	static_tests/node_tuple.cpp
	static_tests/bindable_filter.cpp
	static_tests/count_tuple.cpp
	static_tests/member_traits_tests.cpp
	static_tests/select_return_type.cpp
    static_tests/column.cpp
    static_tests/same_or_void.cpp
    static_tests/foreign_key.cpp
    static_tests/tuple_filter_single.cpp
    static_tests/storage_traits.cpp
    static_tests/tuple_helper.cpp
    static_tests/function_static_tests.cpp
    static_tests/aggregate_function_return_types.cpp
    static_tests/column_result_t.cpp
	constraints/default.cpp
	constraints/unique.cpp
	constraints/foreign_key.cpp
	constraints/check.cpp
	table_tests.cpp
	statement_serializator_tests/primary_key.cpp
	statement_serializator_tests/column_names.cpp
	statement_serializator_tests/autoincrement.cpp
	statement_serializator_tests/arithmetic_operators.cpp
	statement_serializator_tests/core_functions.cpp
	statement_serializator_tests/comparison_operators.cpp
	statement_serializator_tests/unique.cpp
	statement_serializator_tests/foreign_key.cpp
	statement_serializator_tests/collate.cpp
	statement_serializator_tests/check.cpp
	statement_serializator_tests/index.cpp
	statement_serializator_tests/indexed_column.cpp
    statement_serializator_tests/logical_operators.cpp
    statement_serializator_tests/select_constraints.cpp
    statement_serializator_tests/insert_replace.cpp
    statement_serializator_tests/ast/upsert_clause.cpp
    statement_serializator_tests/ast/excluded.cpp
    storage_tests.cpp
    storage_non_crud_tests.cpp
	unique_cases/get_all_with_two_tables.cpp
	unique_cases/prepare_get_all_with_case.cpp
	unique_cases/index_named_table_with_fk.cpp
	unique_cases/issue525.cpp
	unique_cases/delete_with_two_fields.cpp
	unique_cases/join_iterator_ctor_compilation_error.cpp
	get_all_custom_containers.cpp
	select_constraints_tests.cpp
	backup_tests.cpp
	transaction_tests.cpp
    json.cpp
    tuple_helper_tests.cpp
    row_id.cpp
    ast_iterator_tests.cpp
    unique_cases/issue663.cpp
    unique_cases/nonstandart_primary_key.cpp
    static_tests/has_some_type.cpp
    static_tests/is_primary_key_insertable.cpp
    static_tests/is_column_with_insertable_primary_key.cpp)

if(SQLITE_ORM_OMITS_CODECVT)
	message(STATUS "SQLITE_ORM_OMITS_CODECVT is enabled")
	target_compile_definitions(unit_tests PRIVATE SQLITE_ORM_OMITS_CODECVT=1)
endif()

target_link_libraries(unit_tests PRIVATE sqlite_orm Catch2::Catch2)

add_test(NAME "All_in_one_unit_test"
    COMMAND unit_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
