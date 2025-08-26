#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000
using namespace sqlite_orm;

TEST_CASE("virtual table") {
    using Catch::Matchers::UnorderedEquals;

    struct Post {
        std::string title;
        std::string body;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        bool operator==(const Post&) const = default;
#else
        bool operator==(const Post& other) const {
            return this->title == other.title && this->body == other.body;
        }
#endif
    };

    auto virtualTable =
        make_virtual_table("posts", using_fts5(make_column("title", &Post::title), make_column("body", &Post::body)));
    {
        const auto compareColumnName = [](const std::string* foundValue, std::string expectedValue) {
            if (!foundValue) {
                return false;
            }
            return *foundValue == expectedValue;
        };
        REQUIRE(compareColumnName(virtualTable.find_column_name(&Post::title), std::string("title")));
        REQUIRE(compareColumnName(virtualTable.find_column_name(&Post::body), std::string("body")));
    }

    /// CREATE VIRTUAL TABLE posts
    /// USING FTS5(title, body);
    auto storage = make_storage("", std::move(virtualTable));

    storage.sync_schema();
    storage.sync_schema_simulate();
    REQUIRE(storage.table_exists("posts"));

    const std::vector<Post> postsToInsert = {
        Post{"Learn SQlite FTS5", "This tutorial teaches you how to perform full-text search in SQLite using FTS5"},
        Post{"Advanced SQlite Full-text Search", "Show you some advanced techniques in SQLite full-text searching"},
        Post{"SQLite Tutorial", "Help you learn SQLite quickly and effectively"},
    };

    /// INSERT INTO posts(title,body)
    /// VALUES('Learn SQlite FTS5','This tutorial teaches you how to perform full-text search in SQLite using FTS5'),
    /// ('Advanced SQlite Full-text Search','Show you some advanced techniques in SQLite full-text searching'),
    /// ('SQLite Tutorial','Help you learn SQLite quickly and effectively');
    storage.insert_range(postsToInsert.begin(), postsToInsert.end());

    /// SELECT * FROM posts;
    auto posts = storage.get_all<Post>();

    //  check that all the posts are there
    REQUIRE_THAT(posts, UnorderedEquals(postsToInsert));

    /// SELECT *
    /// FROM posts
    /// WHERE posts MATCH 'fts5';
    auto specificPosts = storage.get_all<Post>(where(match<Post>("fts5")));
    decltype(specificPosts) expectedSpecificPosts = {
        {"Learn SQlite FTS5", "This tutorial teaches you how to perform full-text search in SQLite using FTS5"},
    };
    REQUIRE(specificPosts == expectedSpecificPosts);

    ///    SELECT *
    ///    FROM posts
    ///    WHERE posts = 'fts5';
    auto specificPosts2 = storage.get_all<Post>(where(is_equal<Post>("fts5")));
    REQUIRE(specificPosts2 == specificPosts);

    ///    SELECT *
    ///    FROM posts
    ///    WHERE posts MATCH 'text'
    ///    ORDER BY rank;
    auto orderedPosts = storage.get_all<Post>(where(match<Post>("fts5")), order_by(rank()));

    ///    SELECT highlight(posts,0, '<b>', '</b>'),
    ///           highlight(posts,1, '<b>', '</b>')
    ///    FROM posts
    ///    WHERE posts MATCH 'SQLite'
    ///    ORDER BY rank;
    ///
    auto highlightedPosts =
        storage.select(columns(highlight<Post>(0, "<b>", "</b>"), highlight<Post>(1, "<b>", "</b>")),
                       where(match<Post>("SQLite")),
                       order_by(rank()));
}

TEST_CASE("issue1410") {
    struct NormalTable {
        int id;
        std::string text;
        int otherValue;
    };

    struct SearchTable {
        int normal_table_id;
        std::string text;
    };

    auto storage =
        make_storage("",
                     make_table("normal_table",
                                make_column("id", &NormalTable::id, primary_key().autoincrement()),
                                make_column("path", &NormalTable::text),
                                make_column("other_value", &NormalTable::otherValue)),

                     make_virtual_table("search_table",
                                        using_fts5(make_column("text", &SearchTable::text),
                                                   make_column("normal_table_id", &SearchTable::normal_table_id),
                                                   tokenize("trigram"))));
    storage.sync_schema();
    auto rows = storage.iterate<NormalTable>(
        where(eq(&NormalTable::id, select(&SearchTable::normal_table_id, where(match<SearchTable>("Some Text"))))));

    for (const auto& row: rows) {
        std::ignore = row;
    }  //  has to be compiled
}
#endif
