#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer primary key column constraint") {
    std::string value;
    decltype(value) expected;

    using db_objects_t = internal::db_objects_tuple<>;
    auto dbObjects = db_objects_t{};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    SECTION("default pk") {
        constexpr auto pk = primary_key();
        value = serialize(pk, context);
        expected = "PRIMARY KEY";
    }
    SECTION("pk asc") {
        constexpr auto pk = primary_key().asc();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC";
    }
    SECTION("pk asc autoincrement") {
        constexpr auto pk = primary_key().asc().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC AUTOINCREMENT";
    }
    SECTION("pk desc") {
        constexpr auto pk = primary_key().desc();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC";
    }
    SECTION("pk desc autoincrement") {
        constexpr auto pk = primary_key().desc().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC AUTOINCREMENT";
    }
    SECTION("pk on conflict rollback") {
        constexpr auto pk = primary_key().on_conflict_rollback();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT ROLLBACK";
    }
    SECTION("pk on conflict abort") {
        constexpr auto pk = primary_key().on_conflict_abort();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT ABORT";
    }
    SECTION("pk on conflict fail") {
        constexpr auto pk = primary_key().on_conflict_fail();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT FAIL";
    }
    SECTION("pk on conflict ignore") {
        constexpr auto pk = primary_key().on_conflict_ignore();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT IGNORE";
    }
    SECTION("pk on conflict replace") {
        constexpr auto pk = primary_key().on_conflict_replace();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT REPLACE";
    }
    SECTION("pk asc on conflict rollback") {
        constexpr auto pk = primary_key().asc().on_conflict_rollback();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT ROLLBACK";
    }
    SECTION("pk asc on conflict abort") {
        constexpr auto pk = primary_key().asc().on_conflict_abort();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT ABORT";
    }
    SECTION("pk asc on conflict fail") {
        constexpr auto pk = primary_key().asc().on_conflict_fail();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT FAIL";
    }
    SECTION("pk asc on conflict ignore") {
        constexpr auto pk = primary_key().asc().on_conflict_ignore();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT IGNORE";
    }
    SECTION("pk asc on conflict replace") {
        constexpr auto pk = primary_key().asc().on_conflict_replace();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT REPLACE";
    }
    SECTION("pk desc on conflict rollback") {
        constexpr auto pk = primary_key().desc().on_conflict_rollback();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT ROLLBACK";
    }
    SECTION("pk desc on conflict abort") {
        constexpr auto pk = primary_key().desc().on_conflict_abort();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT ABORT";
    }
    SECTION("pk desc on conflict fail") {
        constexpr auto pk = primary_key().desc().on_conflict_fail();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT FAIL";
    }
    SECTION("pk desc on conflict ignore") {
        constexpr auto pk = primary_key().desc().on_conflict_ignore();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT IGNORE";
    }
    SECTION("pk desc on conflict replace") {
        constexpr auto pk = primary_key().desc().on_conflict_replace();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT REPLACE";
    }

    SECTION("pk on conflict rollback autoincrement") {
        constexpr auto pk = primary_key().on_conflict_rollback().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT";
    }
    SECTION("pk on conflict abort autoincrement") {
        constexpr auto pk = primary_key().on_conflict_abort().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT ABORT AUTOINCREMENT";
    }
    SECTION("pk on conflict fail autoincrement") {
        constexpr auto pk = primary_key().on_conflict_fail().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT FAIL AUTOINCREMENT";
    }
    SECTION("pk on conflict ignore autoincrement") {
        constexpr auto pk = primary_key().on_conflict_ignore().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT IGNORE AUTOINCREMENT";
    }
    SECTION("pk on conflict replace autoincrement") {
        constexpr auto pk = primary_key().on_conflict_replace().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ON CONFLICT REPLACE AUTOINCREMENT";
    }
    SECTION("pk asc on conflict rollback autoincrement") {
        constexpr auto pk = primary_key().asc().on_conflict_rollback().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT ROLLBACK AUTOINCREMENT";
    }
    SECTION("pk asc on conflict abort autoincrement") {
        constexpr auto pk = primary_key().asc().on_conflict_abort().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT ABORT AUTOINCREMENT";
    }
    SECTION("pk asc on conflict fail autoincrement") {
        constexpr auto pk = primary_key().asc().on_conflict_fail().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT FAIL AUTOINCREMENT";
    }
    SECTION("pk asc on conflict ignore autoincrement") {
        constexpr auto pk = primary_key().asc().on_conflict_ignore().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT IGNORE AUTOINCREMENT";
    }
    SECTION("pk asc on conflict replace autoincrement") {
        constexpr auto pk = primary_key().asc().on_conflict_replace().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY ASC ON CONFLICT REPLACE AUTOINCREMENT";
    }
    SECTION("pk desc on conflict rollback autoincrement") {
        constexpr auto pk = primary_key().desc().on_conflict_rollback().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT ROLLBACK AUTOINCREMENT";
    }
    SECTION("pk desc on conflict abort autoincrement") {
        constexpr auto pk = primary_key().desc().on_conflict_abort().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT ABORT AUTOINCREMENT";
    }
    SECTION("pk desc on conflict fail autoincrement") {
        constexpr auto pk = primary_key().desc().on_conflict_fail().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT FAIL AUTOINCREMENT";
    }
    SECTION("pk desc on conflict ignore autoincrement") {
        constexpr auto pk = primary_key().desc().on_conflict_ignore().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT IGNORE AUTOINCREMENT";
    }
    SECTION("pk desc on conflict replace autoincrement") {
        constexpr auto pk = primary_key().desc().on_conflict_replace().autoincrement();
        value = serialize(pk, context);
        expected = "PRIMARY KEY DESC ON CONFLICT REPLACE AUTOINCREMENT";
    }
}
