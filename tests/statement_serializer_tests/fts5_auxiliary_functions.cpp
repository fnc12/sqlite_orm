#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
using namespace sqlite_orm;

TEST_CASE("statement_serializer FTS5 auxiliary functions") {
    struct User {
        int id = 0;
        std::string name;

        using hidden = fts5::hidden_fields_of<User>;
    };

    auto table =
        make_virtual_table("users", using_fts5(make_column("id", &User::id), make_column("name", &User::name)));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    std::string value;
    decltype(value) expected;
    SECTION("bm25") {
        auto node = bm25(User::hidden::any_field);
        value = serialize(node, context);
        expected = R"(BM25("users"))";
    }
    SECTION("bm25 with weights") {
        auto node = bm25(User::hidden::any_field, 10.0, 5.0);
        value = serialize(node, context);
        expected = R"(BM25("users", 10, 5))";
    }
    SECTION("snippet") {
        auto node = snippet(User::hidden::any_field, 1, "[", "]", "...", 10);
        value = serialize(node, context);
        expected = R"(SNIPPET("users", 1, '[', ']', '...', 10))";
    }
    REQUIRE(value == expected);
}
#endif
