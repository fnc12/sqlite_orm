#include <sqlite_orm/sqlite_orm.h>
#include <type_traits>  //  std::is_same
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::column_pointer;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
using internal::is_recordset_alias_v;
using internal::is_table_alias_v;
using internal::table_reference;
using internal::using_t;
#endif

template<class T, class E>
void do_assert() {
    STATIC_REQUIRE(std::is_same<T, E>::value);
}

template<class E, class T>
void runTest(const T& /*test*/) {
    do_assert<T, E>();
}

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
template<class C>
concept field_callable = requires(C field) {
    { count(field) };
    { avg(field) };
    { max(field) };
    { min(field) };
    { sum(field) };
    { total(field) };
    { group_concat(field) };
};

template<class S, class C>
concept storage_field_callable = requires(S& storage, C field) {
    { storage.count(field) };
    { storage.avg(field) };
    { storage.max(field) };
    { storage.min(field) };
    { storage.sum(field) };
    { storage.total(field) };
    { storage.group_concat(field) };
    { storage.group_concat(field, "") };
    { storage.group_concat(field, std::string{}) };
    { storage.group_concat(field, 42) };
};

template<class S, orm_table_reference auto mapped>
concept storage_table_reference_callable = requires(S& storage) {
    { storage.get<mapped>(42) };
    { storage.get_all<mapped>() };
    { storage.count<mapped>() };
};
#endif

TEST_CASE("column pointers") {
    struct User {
        int id;
    };
    struct DerivedUser : User {};
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto derived_user = c<DerivedUser>();
#endif

    SECTION("table reference") {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        STATIC_REQUIRE(orm_table_reference<decltype(derived_user)>);
        STATIC_REQUIRE_FALSE(is_table_alias_v<decltype(derived_user)>);
        STATIC_REQUIRE_FALSE(is_recordset_alias_v<decltype(derived_user)>);
        STATIC_REQUIRE_FALSE(orm_table_alias<decltype(derived_user)>);
        STATIC_REQUIRE_FALSE(orm_recordset_alias<decltype(derived_user)>);
        runTest<table_reference<DerivedUser>>(derived_user);
        runTest<DerivedUser>(internal::decay_table_reference_t<derived_user>{});
#endif
    }
    SECTION("column pointer expressions") {
        runTest<column_pointer<User, decltype(&User::id)>>(column<User>(&User::id));
        runTest<column_pointer<DerivedUser, decltype(&DerivedUser::id)>>(column<DerivedUser>(&DerivedUser::id));
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        runTest<column_pointer<DerivedUser, decltype(&DerivedUser::id)>>(derived_user->*&DerivedUser::id);
        STATIC_REQUIRE(field_callable<decltype(&User::id)>);
        STATIC_REQUIRE(field_callable<decltype(derived_user->*&DerivedUser::id)>);

        using storage_type = decltype(make_storage(
            "",
            make_table<User>("user", make_column("id", &User::id, primary_key())),
            make_table<DerivedUser>("derived_user", make_column("id", &DerivedUser::id, primary_key()))));

        STATIC_REQUIRE(storage_field_callable<storage_type, decltype(&User::id)>);
        STATIC_REQUIRE(storage_field_callable<storage_type, decltype(derived_user->*&DerivedUser::id)>);
#endif
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("table reference expressions") {
        runTest<internal::table_t<DerivedUser, false>>(make_table<derived_user>("derived_user"));
        runTest<internal::from_t<DerivedUser>>(from<derived_user>());
        runTest<internal::asterisk_t<DerivedUser>>(asterisk<derived_user>());
        runTest<internal::object_t<DerivedUser>>(object<derived_user>());
        runTest<internal::count_asterisk_t<DerivedUser>>(count<derived_user>());
        runTest<internal::get_t<DerivedUser, int>>(get<derived_user>(42));
        runTest<internal::get_all_t<DerivedUser, std::vector<DerivedUser>>>(get_all<derived_user>());
        runTest<internal::left_join_t<DerivedUser, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            left_join<derived_user>(using_(derived_user->*&DerivedUser::id)));
        runTest<internal::join_t<DerivedUser, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            join<derived_user>(using_(derived_user->*&DerivedUser::id)));
        runTest<internal::left_outer_join_t<DerivedUser, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            left_outer_join<derived_user>(using_(derived_user->*&DerivedUser::id)));
        runTest<internal::inner_join_t<DerivedUser, using_t<DerivedUser, decltype(&DerivedUser::id)>>>(
            inner_join<derived_user>(using_(derived_user->*&DerivedUser::id)));

        using storage_type = decltype(make_storage(
            "",
            make_table<DerivedUser>("derived_user", make_column("id", &DerivedUser::id, primary_key()))));

        STATIC_REQUIRE(storage_table_reference_callable<storage_type, derived_user>);
    }
#endif
}
