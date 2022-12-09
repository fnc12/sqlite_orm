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
        auto dbObjects = db_objects_t{table};
        using context_t = internal::serializer_context<db_objects_t>;
        context_t context{dbObjects};
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
        SECTION("empty pk asc autoincrement") {
            auto pk = primary_key().asc().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC AUTOINCREMENT";
        }
        SECTION("empty pk desc") {
            auto pk = primary_key().desc();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC";
        }
        SECTION("empty pk desc autoincrement") {
            auto pk = primary_key().desc().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC AUTOINCREMENT";
        }
        SECTION("empty pk on conflict rollback") {
            auto pk = primary_key().on_conflict_rollback();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT ROLLBACK";
        }
        SECTION("empty pk on conflict abort") {
            auto pk = primary_key().on_conflict_abort();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT ABORT";
        }
        SECTION("empty pk on conflict fail") {
            auto pk = primary_key().on_conflict_fail();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT FAIL";
        }
        SECTION("empty pk on conflict ignore") {
            auto pk = primary_key().on_conflict_ignore();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT IGNORE";
        }
        SECTION("empty pk on conflict replace") {
            auto pk = primary_key().on_conflict_replace();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT REPLACE";
        }
        SECTION("empty pk asc on conflict rollback") {
            auto pk = primary_key().asc().on_conflict_rollback();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT ROLLBACK";
        }
        SECTION("empty pk asc on conflict abort") {
            auto pk = primary_key().asc().on_conflict_abort();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT ABORT";
        }
        SECTION("empty pk asc on conflict fail") {
            auto pk = primary_key().asc().on_conflict_fail();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT FAIL";
        }
        SECTION("empty pk asc on conflict ignore") {
            auto pk = primary_key().asc().on_conflict_ignore();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT IGNORE";
        }
        SECTION("empty pk asc on conflict replace") {
            auto pk = primary_key().asc().on_conflict_replace();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT REPLACE";
        }
        SECTION("empty pk desc on conflict rollback") {
            auto pk = primary_key().desc().on_conflict_rollback();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT ROLLBACK";
        }
        SECTION("empty pk desc on conflict abort") {
            auto pk = primary_key().desc().on_conflict_abort();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT ABORT";
        }
        SECTION("empty pk desc on conflict fail") {
            auto pk = primary_key().desc().on_conflict_fail();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT FAIL";
        }
        SECTION("empty pk desc on conflict ignore") {
            auto pk = primary_key().desc().on_conflict_ignore();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT IGNORE";
        }
        SECTION("empty pk desc on conflict replace") {
            auto pk = primary_key().desc().on_conflict_replace();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT REPLACE";
        }

        SECTION("empty pk on conflict rollback autoincrement") {
            auto pk = primary_key().on_conflict_rollback().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT";
        }
        SECTION("empty pk on conflict abort autoincrement") {
            auto pk = primary_key().on_conflict_abort().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT ABORT AUTOINCREMENT";
        }
        SECTION("empty pk on conflict fail autoincrement") {
            auto pk = primary_key().on_conflict_fail().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT FAIL AUTOINCREMENT";
        }
        SECTION("empty pk on conflict ignore autoincrement") {
            auto pk = primary_key().on_conflict_ignore().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT IGNORE AUTOINCREMENT";
        }
        SECTION("empty pk on conflict replace autoincrement") {
            auto pk = primary_key().on_conflict_replace().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ON CONFLICT REPLACE AUTOINCREMENT";
        }
        SECTION("empty pk asc on conflict rollback autoincrement") {
            auto pk = primary_key().asc().on_conflict_rollback().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT ROLLBACK AUTOINCREMENT";
        }
        SECTION("empty pk asc on conflict abort autoincrement") {
            auto pk = primary_key().asc().on_conflict_abort().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT ABORT AUTOINCREMENT";
        }
        SECTION("empty pk asc on conflict fail autoincrement") {
            auto pk = primary_key().asc().on_conflict_fail().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT FAIL AUTOINCREMENT";
        }
        SECTION("empty pk asc on conflict ignore autoincrement") {
            auto pk = primary_key().asc().on_conflict_ignore().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT IGNORE AUTOINCREMENT";
        }
        SECTION("empty pk asc on conflict replace autoincrement") {
            auto pk = primary_key().asc().on_conflict_replace().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY ASC ON CONFLICT REPLACE AUTOINCREMENT";
        }
        SECTION("empty pk desc on conflict rollback autoincrement") {
            auto pk = primary_key().desc().on_conflict_rollback().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT ROLLBACK AUTOINCREMENT";
        }
        SECTION("empty pk desc on conflict abort autoincrement") {
            auto pk = primary_key().desc().on_conflict_abort().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT ABORT AUTOINCREMENT";
        }
        SECTION("empty pk desc on conflict fail autoincrement") {
            auto pk = primary_key().desc().on_conflict_fail().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT FAIL AUTOINCREMENT";
        }
        SECTION("empty pk desc on conflict ignore autoincrement") {
            auto pk = primary_key().desc().on_conflict_ignore().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT IGNORE AUTOINCREMENT";
        }
        SECTION("empty pk desc on conflict replace autoincrement") {
            auto pk = primary_key().desc().on_conflict_replace().autoincrement();
            value = serialize(pk, context);
            expected = "PRIMARY KEY DESC ON CONFLICT REPLACE AUTOINCREMENT";
        }
    }
    REQUIRE(value == expected);
}
