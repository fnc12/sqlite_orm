#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer primary key") {
    std::string value;
    decltype(value) expected;
    SECTION("empty") {
        internal::schema_objects<> storage;
        internal::serializer_context<internal::schema_objects<>> context{storage};
        auto pk = primary_key();
        value = serialize(pk, context);
        expected = "PRIMARY KEY";
    }
    SECTION("not empty") {
        struct User {
            int id = 0;
            std::string name;
        };
        auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
        using schema_objects_t = internal::schema_objects<decltype(table)>;
        auto storageImpl = schema_objects_t{table};
        using context_t = internal::serializer_context<schema_objects_t>;
        context_t context{storageImpl};
        SECTION("single column pk") {
            auto pk = primary_key(&User::id);
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("id"))";
        }
        SECTION("double column pk") {
            auto pk = primary_key(&User::id, &User::name);
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("id", "name"))";
        }
        SECTION("empty pk asc") {
            auto pk = primary_key().asc();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC";
        }
        SECTION("empty pk desc") {
            auto pk = primary_key().desc();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC";
        }
    }
    REQUIRE(value == expected);
}
