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
using internal::using_t;

template<class ColAlias, class E>
void do_assert() {
    STATIC_REQUIRE(std::is_same<ColAlias, E>::value);
}

template<class E, class ColAlias>
void runTest(ColAlias /*colRef*/) {
    do_assert<ColAlias, E>();
}

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
template<class S, orm_table_alias auto als>
concept storage_table_alias_callable = requires(S& storage) {
    { storage.get_all<als>() };
    { storage.count<als>() };
};
#endif

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
        constexpr auto a_col = "a"_col;
        runTest<alias_holder<column_alias<'a'>>>(get<a_col>());
        runTest<column_alias<'a'>>(a_col);
        runTest<as_t<column_alias<'a'>, int User::*>>(as<a_col>(&User::id));
        runTest<as_t<column_alias<'a'>, int User::*>>(&User::id >>= a_col);
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
#ifdef SQLITE_ORM_WITH_CTE
        runTest<column_alias<'1'>>(1_colalias);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        using cte_1 = decltype(1_ctealias);
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
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("table alias expressions") {
        constexpr auto derived_user = c<DerivedUser>();
        constexpr auto d_alias = "d"_alias.for_<DerivedUser>();
        using d_alias_type = decltype("d"_alias.for_<DerivedUser>());
        runTest<internal::from_t<d_alias_type>>(from<d_alias>());
        runTest<internal::asterisk_t<d_alias_type>>(asterisk<d_alias>());
        runTest<internal::object_t<d_alias_type>>(object<d_alias>());
        runTest<internal::count_asterisk_t<d_alias_type>>(count<d_alias>());
        runTest<internal::get_all_t<d_alias_type, std::vector<DerivedUser>>>(get_all<d_alias>());
        runTest<internal::left_join_t<d_alias_type, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            left_join<d_alias>(using_(derived_user->*&DerivedUser::id)));
        runTest<internal::join_t<d_alias_type, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            join<d_alias>(using_(derived_user->*&DerivedUser::id)));
        runTest<internal::left_outer_join_t<d_alias_type, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            left_outer_join<d_alias>(using_(derived_user->*&DerivedUser::id)));
        runTest<internal::inner_join_t<d_alias_type, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            inner_join<d_alias>(using_(derived_user->*&DerivedUser::id)));

        using storage_type = decltype(make_storage(
            "",
            make_table<DerivedUser>("derived_user", make_column("id", &DerivedUser::id, primary_key()))));

        STATIC_REQUIRE(storage_table_alias_callable<storage_type, d_alias>);
    }
#endif
}
