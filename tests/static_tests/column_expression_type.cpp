#include <type_traits>  //  std::is_same
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

template<class St, class E, class V>
void runTest(V value) {
    using Type = internal::column_expression_of_t<St, V>;
    static_assert(std::is_same<Type, E>::value, "");
}

TEST_CASE("column_expression_of_t") {
    struct Org {
        // compile-time mapped (via c_v<>)
        int64 id = 0;
        // not compile-time mapped
        int64 boss = 0;
    };

    auto storage =
        make_storage("", make_table("org", make_column("id", c_v<&Org::id>), make_column("boss", &Org::boss)));
    using storage_type = decltype(storage);

    runTest<storage_type, internal::rowid_t>(rowid());
    runTest<storage_type, int64 Org::*>(&Org::id);
    // std::reference_wrapper
    {
        int64 x;
        runTest<storage_type, int64>(std::ref(x));
    }
    runTest<storage_type, std::integral_constant<int64 Org::*, &Org::id>>(internal::ice_t<&Org::id>{});
    runTest<storage_type, internal::ice_t<&Org::id>>(c_v<&Org::id>);
    runTest<storage_type, std::tuple<internal::ice_t<&Org::id>, int64 Org::*>>(columns(c_v<&Org::id>, &Org::boss));
    runTest<storage_type, std::tuple<int64 Org::*, internal::ice_t<&Org::boss>>>(columns(&Org::id, c_v<&Org::boss>));
    runTest<storage_type, std::tuple<internal::ice_t<&Org::id>, int64 Org::*>>(asterisk<Org>());
    runTest<storage_type,
            std::tuple<internal::alias_column_t<alias_a<Org>, internal::ice_t<&Org::id>>,
                       internal::alias_column_t<alias_a<Org>, int64 Org::*>>>(asterisk<alias_a<Org>>());
    // object_t not allowed
    //runTest<storage_type, Org>(object<Org>());
}
