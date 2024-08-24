#include <sqlite_orm/sqlite_orm.h>
#include <type_traits>  //  std::is_same
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <concepts>  //  same_as
#endif
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::column_pointer;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
using internal::count_asterisk_t;
using internal::decay_table_ref_t;
using internal::get_all_optional_t;
using internal::get_all_pointer_t;
using internal::get_all_t;
using internal::get_optional_t;
using internal::get_pointer_t;
using internal::get_t;
using internal::is_recordset_alias_v;
using internal::is_table_alias_v;
using internal::mapped_view;
using internal::remove_all_t;
using internal::remove_t;
using internal::table_reference;
using internal::using_t;
using std::same_as;
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
    count(field);
    avg(field);
    max(field);
    min(field);
    sum(field);
    total(field);
    group_concat(field);
};

template<class S, class C>
concept storage_field_callable = requires(S& storage, C field) {
    { storage.count(field) } -> same_as<int>;
    { storage.avg(field) } -> same_as<double>;
    { storage.max(field) };
    { storage.min(field) };
    { storage.sum(field) };
    { storage.total(field) } -> same_as<double>;
    { storage.group_concat(field) } -> same_as<std::string>;
    { storage.group_concat(field, "") } -> same_as<std::string>;
    { storage.group_concat(field, std::string{}) } -> same_as<std::string>;
    { storage.group_concat(field, 42) } -> same_as<std::string>;
};

template<orm_refers_to_recordset auto recordset, typename T = decltype(recordset)>
concept refers_to_recordset_callable = requires {
    { count<recordset>() } -> same_as<count_asterisk_t<decay_table_ref_t<T>>>;
};

template<orm_refers_to_table auto mapped, typename T = decltype(mapped), typename O = internal::type_t<T>>
concept refers_to_table_callable = requires {
    { get_all<mapped>() } -> same_as<get_all_t<decay_table_ref_t<T>, std::vector<O>>>;
    { count<mapped>() } -> same_as<count_asterisk_t<decay_table_ref_t<T>>>;
};

template<orm_table_reference auto table, typename O = internal::auto_decay_table_ref_t<table>>
concept table_reference_callable = requires {
    { get<table>(42) } -> same_as<get_t<O, int>>;
    { get_pointer<table>(42) } -> same_as<get_pointer_t<O, int>>;
    { get_optional<table>(42) } -> same_as<get_optional_t<O, int>>;
    { get_all<table>() } -> same_as<get_all_t<O, std::vector<O>>>;
    { get_all_pointer<table>() } -> same_as<get_all_pointer_t<O, std::vector<O>>>;
    { get_all_optional<table>() } -> same_as<get_all_optional_t<O, std::vector<O>>>;
    { remove<table>(42) } -> same_as<remove_t<O, int>>;
    { remove_all<table>() } -> same_as<remove_all_t<O>>;
    { count<table>() } -> same_as<count_asterisk_t<O>>;
};

template<class S, orm_refers_to_table auto mapped, typename O = internal::type_t<decltype(mapped)>>
concept storage_refers_to_table_callable = requires(S& storage) {
    { storage.get_all<mapped>() } -> same_as<std::vector<O>>;
    { storage.count<mapped>() } -> same_as<int>;
    { storage.iterate<mapped>() } -> same_as<mapped_view<O, S>>;
};

template<class S, orm_table_reference auto table, typename O = internal::type_t<decltype(table)>>
concept storage_table_reference_callable = requires(S& storage) {
    { storage.get<table>(42) } -> same_as<O>;
    { storage.get_pointer<table>(42) } -> same_as<std::unique_ptr<O>>;
    { storage.get_optional<table>(42) } -> same_as<std::optional<O>>;
    { storage.get_all<table>() } -> same_as<std::vector<O>>;
    { storage.get_all_pointer<table>() } -> same_as<std::vector<std::unique_ptr<O>>>;
    { storage.get_all_optional<table>() } -> same_as<std::vector<std::optional<O>>>;
    { storage.remove<table>(42) } -> same_as<void>;
    { storage.remove_all<table>() } -> same_as<void>;
    { storage.count<table>() } -> same_as<int>;
    { storage.iterate<table>() } -> same_as<mapped_view<O, S>>;
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
        runTest<DerivedUser>(internal::auto_decay_table_ref_t<derived_user>{});
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

        STATIC_REQUIRE(refers_to_recordset_callable<derived_user>);
        STATIC_REQUIRE(refers_to_table_callable<derived_user>);
        STATIC_REQUIRE(table_reference_callable<derived_user>);
        STATIC_REQUIRE(refers_to_recordset_callable<sqlite_master_table>);
        STATIC_REQUIRE(refers_to_table_callable<sqlite_master_table>);
        STATIC_REQUIRE(table_reference_callable<sqlite_master_table>);
        STATIC_REQUIRE(refers_to_recordset_callable<sqlite_schema>);
        STATIC_REQUIRE(refers_to_table_callable<sqlite_schema>);

        using storage_type = decltype(make_storage(
            "",
            make_sqlite_schema_table(),
            make_table<DerivedUser>("derived_user", make_column("id", &DerivedUser::id, primary_key()))));

        STATIC_REQUIRE(storage_refers_to_table_callable<storage_type, derived_user>);
        STATIC_REQUIRE(storage_table_reference_callable<storage_type, derived_user>);
        STATIC_REQUIRE(storage_refers_to_table_callable<storage_type, sqlite_master_table>);
        STATIC_REQUIRE(storage_table_reference_callable<storage_type, sqlite_master_table>);
        STATIC_REQUIRE(storage_refers_to_table_callable<storage_type, sqlite_schema>);
    }
#endif
}
