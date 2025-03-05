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
