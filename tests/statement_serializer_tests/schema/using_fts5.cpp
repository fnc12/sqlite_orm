#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer using_fts5") {
    struct Post {
        std::string title;
        std::string body;
    };
    std::string value;
    std::string expected;
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
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
    REQUIRE(value == expected);
}
