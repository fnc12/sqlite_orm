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

    SECTION("column alias expressions") {
        runTest<alias_holder<column_alias<'a'>>>(get<colalias_a>());
        runTest<as_t<column_alias<'a'>, int User::*>>(as<colalias_a>(&User::id));
        runTest<alias_column_t<alias_z<User>, int User::*>>(alias_column<alias_z<User>>(&User::id));
        runTest<alias_column_t<alias_z<User>, column_pointer<User, int User::*>>>(
            alias_column<alias_z<User>>(column<User>(&User::id)));
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
#endif
#ifdef SQLITE_ORM_WITH_CTE
        runTest<column_alias<'1'>>(1_colalias);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto c_alias = "c"_alias.for_<"1"_cte>();
        runTest<alias_column_t<alias_c<cte_1>, column_pointer<cte_1, alias_holder<column_alias<'a'>>>>>(
            alias_column<c_alias>("a"_col));
        runTest<alias_column_t<alias_c<cte_1>, column_pointer<cte_1, alias_holder<column_alias<'a'>>>>>(
            alias_column<c_alias>("1"_cte->*"a"_col));
        runTest<alias_column_t<alias_c<cte_1>, column_pointer<cte_1, alias_holder<column_alias<'a'>>>>>(
            c_alias->*"a"_col);
        runTest<alias_column_t<alias_c<cte_1>, column_pointer<cte_1, alias_holder<column_alias<'a'>>>>>(
            c_alias->*("1"_cte->*"a"_col));
#endif
#endif
    }
}
