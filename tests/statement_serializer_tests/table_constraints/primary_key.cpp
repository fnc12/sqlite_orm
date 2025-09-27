#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer primary key table constraint") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct DerivedUser : User {};
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_reference auto derived_user = c<DerivedUser>();
#endif

    std::string value;
    decltype(value) expected;
    SECTION("with inheritance") {
        auto table1 = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
        auto table2 = make_table<DerivedUser>("derived_users",
                                              make_column("derived_id", &DerivedUser::id),
                                              make_column("derived_name", &DerivedUser::name));
        using db_objects_t = internal::db_objects_tuple<decltype(table1), decltype(table2)>;
        auto dbObjects = db_objects_t{table1, table2};
        using context_t = internal::serializer_context<db_objects_t>;
        context_t context{dbObjects};
        context.omit_table_name = false;

        SECTION("single column pk") {
            constexpr auto pk = primary_key(&User::id);
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("id"))";
        }
        SECTION("composite pk") {
            constexpr auto pk = primary_key(&User::id, &User::name);
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("id", "name"))";
        }
        SECTION("base, composite pk") {
            constexpr auto pk = primary_key(&User::id, &User::name);
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("id", "name"))";
        }
        SECTION("inheritance, composite fk, column pointer") {
            constexpr auto pk =
                primary_key(column<DerivedUser>(&DerivedUser::id), column<DerivedUser>(&DerivedUser::name));
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("derived_id", "derived_name"))";
        }
        SECTION("inheritance, composite fk, as field") {
            constexpr auto pk = primary_key<DerivedUser>(&DerivedUser::id, &DerivedUser::name);
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("derived_id", "derived_name"))";
        }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        SECTION("inheritance, composite fk, as field") {
            constexpr auto pk = primary_key<derived_user>(&DerivedUser::id, &DerivedUser::name);
            value = serialize(pk, context);
            expected = R"(PRIMARY KEY("derived_id", "derived_name"))";
        }
#endif
        REQUIRE(value == expected);
    }
}
