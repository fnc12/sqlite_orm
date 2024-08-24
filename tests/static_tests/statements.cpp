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
}
