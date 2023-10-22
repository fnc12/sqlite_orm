#include <sqlite_orm/sqlite_orm.h>
#include <type_traits>  //  std::is_same
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::alias_column_t;
using internal::alias_holder;
using internal::as_t;
using internal::column_alias;
using internal::column_pointer;
using internal::recordset_alias;

template<class ColAlias, class E>
void do_assert() {
    STATIC_REQUIRE(std::is_same<ColAlias, E>::value);
}

template<class E, class ColAlias>
void runTest(ColAlias /*colRef*/) {
    do_assert<ColAlias, E>();
}

TEST_CASE("aliases") {
    struct User {
        int id;
    };
    struct DerivedUser : User {};

    SECTION("column alias expressions") {
        runTest<alias_holder<column_alias<'a'>>>(get<colalias_a>());
        runTest<as_t<column_alias<'a'>, int User::*>>(as<colalias_a>(&User::id));
        runTest<alias_column_t<alias_z<User>, int User::*>>(alias_column<alias_z<User>>(&User::id));
        runTest<alias_column_t<alias_z<User>, column_pointer<User, int User::*>>>(
            alias_column<alias_z<User>>(column<User>(&User::id)));
        runTest<alias_column_t<alias_d<DerivedUser>, column_pointer<DerivedUser, int User::*>>>(
            alias_column<alias_d<DerivedUser>>(column<DerivedUser>(&User::id)));
        // must implicitly create column pointer
        runTest<alias_column_t<alias_d<DerivedUser>, column_pointer<DerivedUser, int User::*>>>(
            alias_column<alias_d<DerivedUser>>(&User::id));
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        runTest<alias_holder<column_alias<'a'>>>(get<"a"_col>());
        runTest<column_alias<'a'>>("a"_col);
        runTest<as_t<column_alias<'a'>, int User::*>>(as<"a"_col>(&User::id));
        runTest<as_t<column_alias<'a'>, int User::*>>(&User::id >>= "a"_col);
        runTest<recordset_alias<User, 'a', 'l', 's'>>(alias<'a', 'l', 's'>.for_<User>());
        constexpr auto z_alias = "z"_alias.for_<User>();
        runTest<recordset_alias<User, 'z'>>(z_alias);
        runTest<alias_column_t<alias_z<User>, int User::*>>(alias_column<z_alias>(&User::id));
        runTest<alias_column_t<alias_z<User>, int User::*>>(z_alias->*&User::id);
        runTest<alias_column_t<alias_z<User>, column_pointer<User, int User::*>>>(z_alias->*column<User>(&User::id));
        runTest<alias_column_t<alias_z<User>, column_pointer<User, int User::*>>>(
            alias_column<z_alias>(column<User>(&User::id)));
        constexpr auto d_alias = "d"_alias.for_<DerivedUser>();
        runTest<alias_column_t<alias_d<DerivedUser>, column_pointer<DerivedUser, int User::*>>>(
            alias_column<d_alias>(column<DerivedUser>(&User::id)));
        // must implicitly create column pointer
        runTest<alias_column_t<alias_d<DerivedUser>, column_pointer<DerivedUser, int User::*>>>(
            alias_column<d_alias>(&User::id));
        runTest<alias_column_t<alias_d<DerivedUser>, column_pointer<DerivedUser, int User::*>>>(d_alias->*&User::id);
#endif
    }
}
