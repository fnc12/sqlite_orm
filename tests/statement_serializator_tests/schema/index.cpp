#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator index") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};
    std::string value;
    decltype(value) expected;
    SECTION("simple") {
        auto index = make_index("id_index", &User::id);
        value = internal::serialize(index, context);
        expected = "CREATE INDEX IF NOT EXISTS 'id_index' ON 'users' (\"id\")";
    }
    SECTION("desc") {
        auto index = make_index("idx_users_id", indexed_column(&User::id).desc());
        value = internal::serialize(index, context);
        expected = "CREATE INDEX IF NOT EXISTS 'idx_users_id' ON 'users' (\"id\" DESC)";
    }
    SECTION("asc") {
        auto index = make_index("idx_users_id", indexed_column(&User::id).asc());
        value = internal::serialize(index, context);
        expected = "CREATE INDEX IF NOT EXISTS 'idx_users_id' ON 'users' (\"id\" ASC)";
    }
    SECTION("collate") {
        auto index = make_index("idx_users_id", indexed_column(&User::id).collate("compare"));
        value = internal::serialize(index, context);
        expected = "CREATE INDEX IF NOT EXISTS 'idx_users_id' ON 'users' (\"id\" COLLATE compare)";
    }
    SECTION("collate asc") {
        auto index = make_index("my_index", indexed_column(&User::id).collate("compare").asc());
        value = internal::serialize(index, context);
        expected = "CREATE INDEX IF NOT EXISTS 'my_index' ON 'users' (\"id\" COLLATE compare ASC)";
    }
    REQUIRE(value == expected);
}
