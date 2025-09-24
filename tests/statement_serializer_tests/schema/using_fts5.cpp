#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000
using namespace sqlite_orm;

TEST_CASE("statement_serializer using_fts5") {
    struct Post {
        std::string title;
        std::string body;
    };
    struct User {
        int id = 0;
        std::string name;
    };
    std::string value;
    std::string expected;
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    SECTION("simple") {
        auto node = using_fts5(make_column("title", &Post::title), make_column("body", &Post::body));
        value = serialize(node, context);
        expected = R"(USING FTS5("title", "body"))";
    }
    SECTION("unindexed") {
        auto node = using_fts5(make_column("title", &Post::title), make_column("body", &Post::body, unindexed()));
        value = serialize(node, context);
        expected = R"(USING FTS5("title", "body" UNINDEXED))";
    }
    SECTION("prefix=2") {
        auto node = using_fts5(make_column("title", &Post::title), make_column("body", &Post::body), prefix(2));
        value = serialize(node, context);
        expected = R"(USING FTS5("title", "body", prefix=2))";
    }
    SECTION("tokenize") {
        SECTION("porter ascii") {
            auto node = using_fts5(make_column("title", &Post::title),
                                   make_column("body", &Post::body),
                                   tokenize("porter ascii"));
            value = serialize(node, context);
            expected = R"(USING FTS5("title", "body", tokenize = 'porter ascii'))";
        }
        SECTION("unicode61 remove_diacritics 1") {
            auto node = using_fts5(make_column("title", &Post::title),
                                   make_column("body", &Post::body),
                                   tokenize("unicode61 remove_diacritics 1"));
            value = serialize(node, context);
            expected = R"(USING FTS5("title", "body", tokenize = 'unicode61 remove_diacritics 1'))";
        }
    }
    SECTION("content") {
        auto node = using_fts5(make_column("title", &Post::title), make_column("body", &Post::body), content(""));
        value = serialize(node, context);
        expected = R"(USING FTS5("title", "body", content=''))";
    }
    SECTION("table_content") {
        auto node = using_fts5(make_column("title", &Post::title), make_column("body", &Post::body), content<User>());
        value = serialize(node, context);
        expected = R"(USING FTS5("title", "body", content="users"))";
    }
    REQUIRE(value == expected);
}
#endif
