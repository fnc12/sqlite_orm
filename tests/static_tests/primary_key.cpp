#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::as_field_of_t;
using internal::column_pointer;

TEST_CASE("primary key static") {
    struct Base {
        int id;
        int id2;
    };
    struct Derived : Base {};
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_reference auto derived = c<Derived>();
#endif

    SECTION("inheritance, composite pk") {
        using compositePk1 = decltype(primary_key(column<Derived>(&Derived::id), column<Derived>(&Derived::id2)));
        STATIC_REQUIRE(std::is_same<compositePk1::columns_tuple,
                                    std::tuple<column_pointer<Derived, decltype(&Derived::id)>,
                                               column_pointer<Derived, decltype(&Derived::id2)>>>::value);

        using compositePk2 = decltype(primary_key<Derived>(&Derived::id, &Derived::id2));
        STATIC_REQUIRE(std::is_same<compositePk2::columns_tuple,
                                    std::tuple<as_field_of_t<Derived, decltype(&Derived::id)>,
                                               as_field_of_t<Derived, decltype(&Derived::id2)>>>::value);

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        using compositePk3 = decltype(primary_key<derived>(&Derived::id, &Derived::id2));
        STATIC_REQUIRE(std::is_same<compositePk3::columns_tuple,
                                    std::tuple<as_field_of_t<Derived, decltype(&Derived::id)>,
                                               as_field_of_t<Derived, decltype(&Derived::id2)>>>::value);
#endif
    }
}

TEST_CASE("primary key classification") {
    using internal::is_autoincrement_pk_v;
    using internal::is_column_primary_key_v;
    using internal::is_primary_key_v;

    struct Object {
        int id;
    };

    using ColumnPk = decltype(primary_key());
    using AutoincrementPk = decltype(primary_key().autoincrement());
    using TablePk = decltype(primary_key(&Object::id));

    SECTION("column primary key") {
        STATIC_REQUIRE(is_primary_key_v<ColumnPk>);
        STATIC_REQUIRE(is_column_primary_key_v<ColumnPk>);
        STATIC_REQUIRE_FALSE(is_autoincrement_pk_v<ColumnPk>);
    }
    SECTION("autoincrement") {
        //  AUTOINCREMENT only ever decorates a column primary key, hence such a node is a primary key in its own right
        STATIC_REQUIRE(is_primary_key_v<AutoincrementPk>);
        STATIC_REQUIRE(is_column_primary_key_v<AutoincrementPk>);
        STATIC_REQUIRE(is_autoincrement_pk_v<AutoincrementPk>);
    }
    SECTION("table primary key") {
        STATIC_REQUIRE(is_primary_key_v<TablePk>);
        STATIC_REQUIRE_FALSE(is_column_primary_key_v<TablePk>);
        STATIC_REQUIRE_FALSE(is_autoincrement_pk_v<TablePk>);
    }
    SECTION("other constraint") {
        STATIC_REQUIRE_FALSE(is_primary_key_v<decltype(unique())>);
        STATIC_REQUIRE_FALSE(is_autoincrement_pk_v<decltype(unique())>);
    }
}
