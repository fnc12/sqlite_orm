#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer index") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
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
#ifdef SQLITE_ENABLE_JSON1
    SECTION("json") {
        SECTION("implicit") {
            auto index = make_index<User>("idx", json_extract<bool>(&User::name, "$.field"));
            value = internal::serialize(index, context);
        }
        SECTION("explicit") {
            auto index = make_index<User>("idx", indexed_column(json_extract<bool>(&User::name, "$.field")));
            value = internal::serialize(index, context);
        }
        expected = "CREATE INDEX IF NOT EXISTS \"idx\" ON \"users\" (JSON_EXTRACT(\"name\", '$.field'))";
    }
#endif  //  SQLITE_ENABLE_JSON1
    REQUIRE(value == expected);
}
