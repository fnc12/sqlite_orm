#include <type_traits>  //  std::is_same
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::alias_column_t;
using internal::alias_holder;
using internal::column_alias;
using internal::column_pointer;

template<class Type, class E>
void do_assert() {
    STATIC_REQUIRE(std::is_same<Type, E>::value);
}

template<class DBOs, class E, class V>
void runTest(V /*value*/) {
    do_assert<internal::column_expression_of_t<DBOs, V>, E>();
}

TEST_CASE("column_expression_of_t") {
    struct Org {
        int64 id = 0;
        int64 boss = 0;
    };
    struct Derived : Org {};

    auto dbObjects = std::make_tuple(
        make_table("org", make_column("id", &Org::id), make_column("boss", &Org::boss)),
        make_table<Derived>("derived", make_column("id", &Derived::id), make_column("boss", &Derived::boss)));
    using db_objects_t = decltype(dbObjects);

    runTest<db_objects_t, int>(1);
    runTest<db_objects_t, internal::rowid_t>(rowid());
    runTest<db_objects_t, int64 Org::*>(&Org::id);
    // std::reference_wrapper
    {
        const int64 x = 42;
        runTest<db_objects_t, const int64&>(std::ref(x));
    }
    runTest<db_objects_t, std::tuple<int64 Org::*>>(columns(&Org::boss));
    runTest<db_objects_t, std::tuple<int64 Org::*, int64 Org::*>>(asterisk<Org>());
    runTest<db_objects_t,
            std::tuple<alias_column_t<alias_a<Org>, int64 Org::*>, alias_column_t<alias_a<Org>, int64 Org::*>>>(
        asterisk<alias_a<Org>>());
    // object_t not allowed
    //runTest<db_objects_t, Org>(object<Org>());
    runTest<db_objects_t, internal::as_t<colalias_a, int>>(as<colalias_a>(1));
    runTest<db_objects_t, alias_column_t<alias_a<Org>, int64 Org::*>>(alias_column<alias_a<Org>>(&Org::id));

    runTest<db_objects_t, column_pointer<Derived, int64 Org::*>>(column<Derived>(&Org::id));
#ifdef SQLITE_ORM_WITH_CTE
    auto dbObjects2 = internal::storage_db_objects_cat(
        dbObjects,
        internal::make_cte_table(dbObjects, cte<cte_1>()(select(columns(&Org::id, 1)))));
    using db_objects2_t = decltype(dbObjects2);
    runTest<db_objects_t, column_pointer<cte_1, int64 Org::*>>(column<cte_1>(&Org::id));
    runTest<db_objects_t, column_pointer<cte_1, int64 Org::*>>(column<cte_1>->*&Org::id);
    runTest<db_objects_t, column_pointer<cte_1, int64 Org::*>>(cte_1{}->*&Org::id);
    runTest<db_objects_t, column_pointer<cte_1, alias_holder<column_alias<'1'>>>>(column<cte_1>(1_colalias));
    runTest<db_objects_t, column_pointer<cte_1, alias_holder<colalias_c>>>(1_ctealias->*colalias_c{});
    runTest<db_objects_t, alias_column_t<alias_a<cte_1>, column_pointer<cte_1, int64 Org::*>>>(
        alias_column<alias_a<cte_1>>(&Org::id));
    runTest<db_objects_t, alias_column_t<alias_a<cte_1>, column_pointer<cte_1, alias_holder<column_alias<'1'>>>>>(
        alias_column<alias_a<cte_1>>(1_colalias));
    runTest<db_objects2_t, std::tuple<int64 Org::*, int internal::aliased_field<column_alias<'2'>, int>::*>>(
        asterisk<cte_1>());
    runTest<db_objects2_t,
            std::tuple<alias_column_t<alias_a<cte_1>, int64 Org::*>,
                       alias_column_t<alias_a<cte_1>, int internal::aliased_field<column_alias<'2'>, int>::*>>>(
        asterisk<alias_a<cte_1>>());
#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
    runTest<db_objects_t, column_pointer<cte_1, alias_holder<colalias_c>>>("1"_cte->*"c"_col);
#endif
#endif
}
