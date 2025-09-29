#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000
using namespace sqlite_orm;

TEST_CASE("statement_serializer fts5") {
    struct Post {
        std::string title;
        std::string body;
    };
    struct User {
        int id = 0;
        std::string name;
    };
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_reference auto post = c<Post>();
    constexpr orm_table_reference auto user = c<User>();
#endif

    std::string value;
    std::string expected;
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    SECTION("simple") {
        auto expression =
            make_virtual_table("posts",
                               using_fts5(make_column("title", &Post::title), make_column("body", &Post::body)));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body"))";
    }
    SECTION("explicit object") {
        auto expression =
            make_virtual_table("posts",
                               using_fts5<Post>(make_column("title", &Post::title), make_column("body", &Post::body)));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body"))";
    }
    SECTION("unindexed") {
        auto expression = make_virtual_table(
            "posts",
            using_fts5(make_column("title", &Post::title), make_column("body", &Post::body, unindexed())));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body" UNINDEXED))";
    }
    SECTION("prefix=2") {
        auto expression = make_virtual_table(
            "posts",
            using_fts5(make_column("title", &Post::title), make_column("body", &Post::body), prefix(2)));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body", prefix=2))";
    }
    SECTION("tokenize") {
        SECTION("porter ascii") {
            auto expression = make_virtual_table("posts",
                                                 using_fts5(make_column("title", &Post::title),
                                                            make_column("body", &Post::body),
                                                            tokenize("porter ascii")));
            value = serialize(expression, context);
            expected =
                R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body", tokenize = 'porter ascii'))";
        }
        SECTION("unicode61 remove_diacritics 1") {
            auto expression = make_virtual_table("posts",
                                                 using_fts5(make_column("title", &Post::title),
                                                            make_column("body", &Post::body),
                                                            tokenize("unicode61 remove_diacritics 1")));
            value = serialize(expression, context);
            expected =
                R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body", tokenize = 'unicode61 remove_diacritics 1'))";
        }
    }
    SECTION("content") {
        auto expression = make_virtual_table(
            "posts",
            using_fts5(make_column("title", &Post::title), make_column("body", &Post::body), content("")));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body", content=''))";
    }
    SECTION("table_content") {
        auto expression = make_virtual_table(
            "posts",
            using_fts5(make_column("title", &Post::title), make_column("body", &Post::body), content<User>()));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body", content="users"))";
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("table reference") {
        auto expression = make_virtual_table(
            "posts",
            using_fts5<post>(make_column("title", &Post::title), make_column("body", &Post::body), content<user>()));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "posts" USING "fts5"("title", "body", content="users"))";
    }
#endif
    REQUIRE(value == expected);
}
#endif
