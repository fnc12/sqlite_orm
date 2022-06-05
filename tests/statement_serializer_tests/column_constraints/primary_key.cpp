#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer primary key") {
    std::string value;
    decltype(value) expected;
    SECTION("empty") {
        internal::db_objects_tuple<> storage;
        internal::serializer_context<internal::db_objects_tuple<>> context{storage};
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
        using db_objects_t = internal::db_objects_tuple<decltype(table)>;
        auto storageImpl = db_objects_t{table};
        using context_t = internal::serializer_context<db_objects_t>;
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
