#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Generated") {
    struct A {
        int a;
        int b;
        std::string c;
        int d;
        std::string e;
    };

    auto storage = make_storage({}, make_table("A",
                                           make_column("a", &A::a, primary_key()),
                                           make_column("b", &A::b),
                                           make_column("c", &A::c),
                                           make_column("d", &A::d),
                                           make_column("e", &A::e, generated_always().as(mul(&A::a, abs(&A::b))).virtual_())));
    storage.sync_schema();

    // TODO: validate generated values

}