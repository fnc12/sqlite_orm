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
        expected = "USING FTS5(\"title\", \"body\")";
    }
    SECTION("unindexed") {
        auto node = using_fts5(make_column("title", &Post::title), make_column("body", &Post::body, unindexed()));
        value = serialize(node, context);
        expected = "USING FTS5(\"title\", \"body\" UNINDEXED)";
    }
    REQUIRE(value == expected);
}
