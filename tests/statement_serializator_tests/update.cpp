#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator update") {
    using internal::serialize;
    struct A {
        int address = 0;
        int type = 0;
        int index = 0;
        double value = 0;
    };
    std::string value;
    decltype(value) expected;
    SECTION("primary key") {
        SECTION("column") {
            auto table = make_table("table",
                                    make_column("address", &A::address, primary_key()),
                                    make_column("type", &A::type),
                                    make_column("idx", &A::index),
                                    make_column("value", &A::value));
            using storage_impl_t = internal::storage_impl<decltype(table)>;
            auto storageImpl = storage_impl_t{table};
            using context_t = internal::serializator_context<storage_impl_t>;
            context_t context{storageImpl};

            A object{1, 2, 3, 4};
            auto expression = update(object);
            value = serialize(expression, context);
        }
        SECTION("table") {
            auto table = make_table("table",
                                    make_column("address", &A::address),
                                    make_column("type", &A::type),
                                    make_column("idx", &A::index),
                                    make_column("value", &A::value),
                                    primary_key(&A::address));
            using storage_impl_t = internal::storage_impl<decltype(table)>;
            auto storageImpl = storage_impl_t{table};
            using context_t = internal::serializator_context<storage_impl_t>;
            context_t context{storageImpl};

            A object{1, 2, 3, 4};
            auto expression = update(object);
            value = serialize(expression, context);
        }
        expected = "UPDATE 'table' SET \"type\" = ?, \"idx\" = ?, \"value\" = ? WHERE \"address\" = ?";
    }
    SECTION("composite key 2") {
        auto table = make_table("table",
                                make_column("address", &A::address),
                                make_column("type", &A::type),
                                make_column("idx", &A::index),
                                make_column("value", &A::value),
                                primary_key(&A::address, &A::type));
        using storage_impl_t = internal::storage_impl<decltype(table)>;
        auto storageImpl = storage_impl_t{table};
        using context_t = internal::serializator_context<storage_impl_t>;
        context_t context{storageImpl};

        A object{1, 2, 3, 4};
        auto expression = update(object);
        value = serialize(expression, context);
        expected = "UPDATE 'table' SET \"idx\" = ?, \"value\" = ? WHERE \"address\" = ? AND \"type\" = ?";
    }
    SECTION("composite key 3") {
        auto table = make_table("table",
                                make_column("address", &A::address),
                                make_column("type", &A::type),
                                make_column("idx", &A::index),
                                make_column("value", &A::value),
                                primary_key(&A::address, &A::type, &A::index));
        using storage_impl_t = internal::storage_impl<decltype(table)>;
        auto storageImpl = storage_impl_t{table};
        using context_t = internal::serializator_context<storage_impl_t>;
        context_t context{storageImpl};

        A object{1, 2, 3, 4};
        auto expression = update(object);
        value = serialize(expression, context);
        expected = "UPDATE 'table' SET \"value\" = ? WHERE \"address\" = ? AND \"type\" = ? AND \"idx\" = ?";
    }
    REQUIRE(value == expected);
}
