#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::expression_object_type_t;
using internal::statement_object_type_t;

template<class Expected, class Expression>
static void runExpressionTest(Expression /*expression*/) {
    using Statement = internal::prepared_statement_t<Expression>;
    STATIC_REQUIRE(std::is_same<expression_object_type_t<Expression>, Expected>::value);
    STATIC_REQUIRE(std::is_same<statement_object_type_t<Statement>, Expected>::value);
}

TEST_CASE("statements") {
    struct Object {
        int64 id;
    };
    constexpr Object obj{};
    constexpr std::array<Object, 0> objs{};

    SECTION("expression object") {
        runExpressionTest<Object>(insert(obj));
        runExpressionTest<Object>(insert(std::cref(obj)));
        runExpressionTest<Object>(insert(obj, columns(&Object::id)));
        runExpressionTest<Object>(insert(std::cref(obj), columns(&Object::id)));
        runExpressionTest<Object>(insert_range(objs.cbegin(), objs.cend()));
        runExpressionTest<Object>(replace(obj));
        runExpressionTest<Object>(replace(std::cref(obj)));
        runExpressionTest<Object>(replace_range(objs.cbegin(), objs.cend()));
        runExpressionTest<Object>(update(obj));
        runExpressionTest<Object>(update(std::cref(obj)));
        runExpressionTest<Object>(remove<Object>(0));
    }

    SECTION("grammar classification") {
        //  every DSL spelling of a DML statement is classified as itself, ...
        STATIC_REQUIRE(internal::is_insert_v<decltype(insert(obj))>);
        STATIC_REQUIRE(internal::is_insert_explicit_v<decltype(insert(obj, columns(&Object::id)))>);
        STATIC_REQUIRE(internal::is_insert_range_v<decltype(insert_range(objs.cbegin(), objs.cend()))>);
        STATIC_REQUIRE(internal::is_insert_raw_v<decltype(insert(into<Object>(), default_values()))>);
        STATIC_REQUIRE(internal::is_replace_v<decltype(replace(obj))>);
        STATIC_REQUIRE(internal::is_replace_range_v<decltype(replace_range(objs.cbegin(), objs.cend()))>);
        STATIC_REQUIRE(internal::is_replace_raw_v<decltype(replace(into<Object>(), default_values()))>);
        STATIC_REQUIRE(internal::is_update_v<decltype(update(obj))>);
        STATIC_REQUIRE(internal::is_update_all_v<decltype(update_all(set(c(&Object::id) = 0)))>);
        STATIC_REQUIRE(internal::is_remove_v<decltype(remove<Object>(0))>);
        STATIC_REQUIRE(internal::is_remove_all_v<decltype(remove_all<Object>())>);
        STATIC_REQUIRE(internal::is_insert_constraint_v<decltype(or_abort())>);
        STATIC_REQUIRE(internal::is_default_values_v<decltype(default_values())>);

        //  ... and as nothing else - the spellings of one SQL production stay distinguishable
        STATIC_REQUIRE_FALSE(internal::is_insert_v<decltype(insert(obj, columns(&Object::id)))>);
        STATIC_REQUIRE_FALSE(internal::is_insert_v<decltype(insert(into<Object>(), default_values()))>);
        STATIC_REQUIRE_FALSE(internal::is_insert_raw_v<decltype(insert(obj))>);
        STATIC_REQUIRE_FALSE(internal::is_replace_v<decltype(replace(into<Object>(), default_values()))>);
        STATIC_REQUIRE_FALSE(internal::is_remove_v<decltype(remove_all<Object>())>);
        STATIC_REQUIRE_FALSE(internal::is_remove_all_v<decltype(remove<Object>(0))>);
        STATIC_REQUIRE_FALSE(internal::is_update_v<decltype(update_all(set(c(&Object::id) = 0)))>);
        STATIC_REQUIRE_FALSE(internal::is_update_all_v<decltype(update(obj))>);
        STATIC_REQUIRE_FALSE(internal::is_insert_v<int>);
        STATIC_REQUIRE_FALSE(internal::is_insert_constraint_v<decltype(default_values())>);
        STATIC_REQUIRE_FALSE(internal::is_default_values_v<decltype(or_abort())>);

        //  the clauses only a DML statement takes are classified alongside the statements
        STATIC_REQUIRE(internal::is_into_v<decltype(into<Object>())>);
        STATIC_REQUIRE(internal::is_values_v<decltype(values(std::make_tuple(0)))>);
        STATIC_REQUIRE(internal::is_dynamic_values_v<decltype(values(std::vector<int>{}))>);
        STATIC_REQUIRE_FALSE(internal::is_values_v<decltype(values(std::vector<int>{}))>);
        //  ... but both are the one VALUES production, which is what a raw INSERT accepts
        STATIC_REQUIRE(internal::is_any_values_v<decltype(values(std::make_tuple(0)))>);
        STATIC_REQUIRE(internal::is_any_values_v<decltype(values(std::vector<int>{}))>);
        STATIC_REQUIRE(internal::is_set_v<decltype(set(c(&Object::id) = 0))>);
        STATIC_REQUIRE(internal::is_any_set_v<decltype(set(c(&Object::id) = 0))>);
        STATIC_REQUIRE_FALSE(internal::is_dynamic_set_v<decltype(set(c(&Object::id) = 0))>);
        STATIC_REQUIRE_FALSE(internal::is_into_v<decltype(values(std::make_tuple(0)))>);
        STATIC_REQUIRE_FALSE(internal::is_upsert_clause_v<decltype(into<Object>())>);
    }

    SECTION("semantic classification") {
        //  an INSERT with an explicit column list operates on an object, just like the other object spellings
        STATIC_REQUIRE(internal::is_object_dml_expression_v<decltype(insert(obj, columns(&Object::id)))>);
        //  the range spellings are not - they carry no single object to reach through `access_dml_object()`
        STATIC_REQUIRE_FALSE(internal::is_object_dml_expression_v<decltype(insert_range(objs.cbegin(), objs.cend()))>);
        STATIC_REQUIRE_FALSE(internal::is_object_dml_expression_v<decltype(replace_range(objs.cbegin(), objs.cend()))>);
        STATIC_REQUIRE(internal::is_raw_dml_expression_v<decltype(insert(into<Object>(), default_values()))>);
        STATIC_REQUIRE(internal::is_raw_dml_expression_v<decltype(replace(into<Object>(), default_values()))>);
    }
}
