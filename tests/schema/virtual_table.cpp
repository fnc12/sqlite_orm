#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("virtual table") {
    using Catch::Matchers::UnorderedEquals;

    struct Post {
        std::string title;
        std::string body;

        bool operator==(const Post& other) const {
            return this->title == other.title && this->body == other.body;
        }
    };

    /// CREATE VIRTUAL TABLE posts
    /// USING FTS5(title, body);
    auto storage = make_storage(
        "",
        make_virtual_table("posts", using_fts5(make_column("title", &Post::title), make_column("body", &Post::body))));

    storage.sync_schema();
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
