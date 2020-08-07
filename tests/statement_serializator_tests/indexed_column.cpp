#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator indexed_column") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};
    {
        auto column = indexed_column(&User::id);
        auto value = internal::serialize(column, context);
        REQUIRE(value == "\"id\"");
    }
    {
        auto column = indexed_column(&User::id).asc();
        auto value = internal::serialize(column, context);
        REQUIRE(value == "\"id\" ASC");
    }
    {
        auto column = indexed_column(&User::id).desc();
        auto value = internal::serialize(column, context);
        REQUIRE(value == "\"id\" DESC");
    }
    {
        auto column = indexed_column(&User::id).collate("BINARY");
        auto value = internal::serialize(column, context);
        REQUIRE(value == "\"id\" COLLATE BINARY");
    }
    {
        auto column = indexed_column(&User::name).collate("BINARY").asc();
        auto value = internal::serialize(column, context);
        REQUIRE(value == "\"name\" COLLATE BINARY ASC");
    }
    {
        auto column = indexed_column(&User::name).collate("OTHER").desc();
        auto value = internal::serialize(column, context);
        REQUIRE(value == "\"name\" COLLATE OTHER DESC");
    }
}
