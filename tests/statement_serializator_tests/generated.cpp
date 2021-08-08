#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator generated") {
    using storage_impl_t = internal::storage_impl<>;
    storage_impl_t storageImpl;
    internal::serializator_context<storage_impl_t> context{storageImpl};
    SECTION("generated_always_as") {
        auto statement = generated_always().as(1);
        auto stringValue = internal::serialize(statement, context);
        REQUIRE(stringValue == "GENERATED ALWAYS AS(1)");

        SECTION("VIRTUAL") {
            auto statementVirtual = statement.virtual_();
            stringValue = internal::serialize(statementVirtual, context);
            REQUIRE(stringValue == "GENERATED ALWAYS AS(1) VIRTUAL");
        }
        SECTION("STORED") {
            auto statementStored = statement.stored();
            stringValue = internal::serialize(statementStored, context);
            REQUIRE(stringValue == "GENERATED ALWAYS AS(1) STORED");
        }
    }
    SECTION("as") {
        auto statement = as(1);
        auto stringValue = internal::serialize(statement, context);
        REQUIRE(stringValue == "AS(1)");

        SECTION("VIRTUAL") {
            auto statementVirtual = statement.virtual_();
            stringValue = internal::serialize(statementVirtual, context);
            REQUIRE(stringValue == "AS(1) VIRTUAL");
        }
        SECTION("STORED") {
            auto statementStored = statement.stored();
            stringValue = internal::serialize(statementStored, context);
            REQUIRE(stringValue == "AS(1) STORED");
        }
    }
}