#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer index") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializer_context<storage_impl_t>;
    context_t context{storageImpl};
    std::string value;
    decltype(value) expected;
    SECTION("simple") {
        auto index = make_index("id_index", &User::id);
        value = internal::serialize(index, context);
        expected = R"(CREATE INDEX IF NOT EXISTS "id_index" ON "users" ("id"))";
    }
    SECTION("desc") {
        auto index = make_index("idx_users_id", indexed_column(&User::id).desc());
        value = internal::serialize(index, context);
        expected = R"(CREATE INDEX IF NOT EXISTS "idx_users_id" ON "users" ("id" DESC))";
    }
    SECTION("asc") {
        auto index = make_index("idx_users_id", indexed_column(&User::id).asc());
        value = internal::serialize(index, context);
        expected = R"(CREATE INDEX IF NOT EXISTS "idx_users_id" ON "users" ("id" ASC))";
    }
    SECTION("collate") {
        auto index = make_index("idx_users_id", indexed_column(&User::id).collate("compare"));
        value = internal::serialize(index, context);
        expected = R"(CREATE INDEX IF NOT EXISTS "idx_users_id" ON "users" ("id" COLLATE compare))";
    }
    SECTION("collate asc") {
        auto index = make_index("my_index", indexed_column(&User::id).collate("compare").asc());
        value = internal::serialize(index, context);
        expected = R"(CREATE INDEX IF NOT EXISTS "my_index" ON "users" ("id" COLLATE compare ASC))";
    }
    SECTION("ifnull") {
        auto index = make_index("idx", &User::id, ifnull<std::string>(&User::name, ""));
        value = internal::serialize(index, context);
        expected = R"(CREATE INDEX IF NOT EXISTS "idx" ON "users" ("id", IFNULL("name", '')))";
    }
    SECTION("where") {
        auto index = make_index("idx", &User::id, where(is_not_null(&User::id)));
        value = internal::serialize(index, context);
        expected = R"(CREATE INDEX IF NOT EXISTS "idx" ON "users" ("id") WHERE ("id" IS NOT NULL))";
    }
    REQUIRE(value == expected);
}

TEST_CASE("trying schema_status index") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storage = make_storage("",
                                make_index("user_name", &User::name),
                                make_table("users",
                                           make_column("id", &User::id, primary_key(), autoincrement()),
                                           make_column("name", &User::name)));
    storage.sync_schema(true);
    storage.sync_schema_simulate(true);  // this will call in storage_t:

    // template<class... Tss, class... Cols>
    // sync_schema_result schema_status(const storage_impl<index_t<Cols...>, Tss...>&, sqlite3*, bool) {
    //     return sync_schema_result::already_in_sync;
    // }
}
