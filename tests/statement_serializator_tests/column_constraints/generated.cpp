#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
#if SQLITE_VERSION_NUMBER >= 3031000
TEST_CASE("statement_serializator generated") {
    using internal::serialize;
    struct Type {
        int a = 0;
        int b = 0;
        std::string c;
        int d = 0;
        std::string e;
    };
    auto table = make_table(
        "t1",
        make_column("a", &Type::a, primary_key()),
        make_column("b", &Type::b),
        make_column("c", &Type::c),
        make_column("d", &Type::d, generated_always_as(&Type::a * sqlite_orm::abs(&Type::b)).virtual_()),
        make_column("e", &Type::e, generated_always_as(substr(&Type::c, &Type::b, add(&Type::b, 1))).stored()));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};
    std::string value;
    decltype(value) expected;
    SECTION("full") {
        auto constraint = generated_always_as(&Type::a * sqlite_orm::abs(&Type::b));
        value = serialize(constraint, context);
        expected = "GENERATED ALWAYS AS (\"a\" * ABS(\"b\"))";
    }
    SECTION("full virtual") {
        auto constraint = generated_always_as(&Type::a * sqlite_orm::abs(&Type::b)).virtual_();
        value = serialize(constraint, context);
        expected = "GENERATED ALWAYS AS (\"a\" * ABS(\"b\")) VIRTUAL";
    }
    SECTION("full stored") {
        auto constraint = generated_always_as(&Type::a * sqlite_orm::abs(&Type::b)).stored();
        value = serialize(constraint, context);
        expected = "GENERATED ALWAYS AS (\"a\" * ABS(\"b\")) STORED";
    }
    SECTION("not full") {
        auto constraint = as(&Type::a * sqlite_orm::abs(&Type::b));
        value = serialize(constraint, context);
        expected = "AS (\"a\" * ABS(\"b\"))";
    }
    SECTION("not full virtual") {
        auto constraint = as(&Type::a * sqlite_orm::abs(&Type::b)).virtual_();
        value = serialize(constraint, context);
        expected = "AS (\"a\" * ABS(\"b\")) VIRTUAL";
    }
    SECTION("not full stored") {
        auto constraint = as(&Type::a * sqlite_orm::abs(&Type::b)).stored();
        value = serialize(constraint, context);
        expected = "AS (\"a\" * ABS(\"b\")) STORED";
    }
    REQUIRE(value == expected);
}
#endif
