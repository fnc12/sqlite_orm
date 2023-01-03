#include <type_traits>  //  std::is_same
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
using internal::alias_holder;
using internal::column_pointer;

template<class St, class E, class V>
void runTest(V /*value*/) {
    using Type = internal::column_expression_of_t<typename St::db_objects_type, V>;
    STATIC_REQUIRE(std::is_same<Type, E>::value);
}

TEST_CASE("column_expression_of_t") {
    struct Org {
        int64 id = 0;
        int64 boss = 0;
    };
    struct Derived : Org {};

    auto storage = make_storage(
        "",
        make_table("org", make_column("id", &Org::id), make_column("boss", &Org::boss)),
        make_table<Derived>("derived", make_column("id", &Derived::id), make_column("boss", &Derived::boss)));
    using storage_type = decltype(storage);

    runTest<storage_type, int>(1);
    runTest<storage_type, internal::rowid_t>(rowid());
    runTest<storage_type, int64 Org::*>(&Org::id);
    // std::reference_wrapper
    {
        const int64 x = 42;
        runTest<storage_type, const int64&>(std::ref(x));
    }
    runTest<storage_type, std::tuple<int64 Org::*>>(columns(&Org::boss));
    runTest<storage_type, std::tuple<int64 Org::*, int64 Org::*>>(asterisk<Org>());
    runTest<storage_type,
            std::tuple<internal::alias_column_t<alias_a<Org>, int64 Org::*>,
                       internal::alias_column_t<alias_a<Org>, int64 Org::*>>>(asterisk<alias_a<Org>>());
    // object_t not allowed
    //runTest<storage_type, Org>(object<Org>());
    runTest<storage_type, internal::as_t<colalias_a, int>>(as<colalias_a>(1));

    runTest<storage_type, column_pointer<Derived, int64 Org::*>>(column<Derived>(&Org::id));
    runTest<storage_type, column_pointer<cte_1, int64 Org::*>>(column<cte_1>(&Org::id));
    runTest<storage_type, column_pointer<cte_1, polyfill::index_constant<0u>>>(column<cte_1>(0_colidx));
    runTest<storage_type, column_pointer<cte_1, int64 Org::*>>(column<cte_1>()->*&Org::id);
    runTest<storage_type, column_pointer<cte_1, int64 Org::*>>(column<cte_1>->*&Org::id);
    runTest<storage_type, column_pointer<cte_1, int64 Org::*>>(cte_1{}->*&Org::id);
    runTest<storage_type, column_pointer<cte_1, alias_holder<internal::column_alias<'1'>>>>(column<cte_1>(1_colalias));
    runTest<storage_type, column_pointer<cte_1, alias_holder<colalias_c>>>(1_ctealias->*colalias_c{});
#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
    runTest<storage_type, column_pointer<cte_1, alias_holder<colalias_c>>>("1"_cte->*"c"_col);
#endif
}
