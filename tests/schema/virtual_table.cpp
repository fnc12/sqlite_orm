#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000
using namespace sqlite_orm;

TEST_CASE("fts5 virtual table schema") {
    using Catch::Matchers::UnorderedEquals;

    struct Post {
        std::string title;
        std::string body;

        // A clever way of defining and using explicit column pointers for hidden `fts5` member fields
        using hidden = fts5::hidden_columns_for<Post>;

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
        constexpr auto compareColumnName = [](const std::string* foundValue, std::string expectedValue) {
            if (!foundValue) {
                return false;
            }
            return *foundValue == expectedValue;
        };
        REQUIRE(compareColumnName(virtualTable.find_column_name(&Post::title), std::string("title")));
        REQUIRE(compareColumnName(virtualTable.find_column_name(&Post::body), std::string("body")));
        REQUIRE(compareColumnName(virtualTable.find_column_name(&fts5::hidden::rank), std::string("rank")));
    }

    /// CREATE VIRTUAL TABLE posts
    /// USING FTS5(title, body);
    auto storage = make_storage("", std::move(virtualTable));

    storage.sync_schema();
    storage.sync_schema_simulate();
    REQUIRE(storage.table_exists("posts"));

    const std::vector<Post> postsToInsert = {
        {"Learn SQlite FTS5", "This tutorial teaches you how to perform full-text search in SQLite using FTS5"},
        {"Advanced SQlite Full-text Search", "Show you some advanced techniques in SQLite full-text searching"},
        {"SQLite Tutorial", "Help you learn SQLite quickly and effectively"},
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
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    orderedPosts = storage.get_all<Post>(where(match<Post>("fts5")), order_by(Post::hidden::rank_column));
#endif

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
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    highlightedPosts = storage.select(columns(highlight<Post>(0, "<b>", "</b>"), highlight<Post>(1, "<b>", "</b>")),
                                      where(match<Post>("SQLite")),
                                      order_by(Post::hidden::rank_column));
#endif
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
    }  // must compile
}
#endif

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("dbstat virtual table schema") {
    constexpr auto compareColumnName = [](const std::string* foundValue, const std::string& expectedValue) {
        if (!foundValue) {
            return false;
        }
        return *foundValue == expectedValue;
    };

    SECTION("epynomous") {
        SECTION("definition") {
            auto virtualTable = make_dbstat_table();
            REQUIRE(compareColumnName(virtualTable.find_column_name(&dbstat::name), "name"));
            REQUIRE(compareColumnName(virtualTable.find_column_name(&dbstat::pgsize), "pgsize"));
            REQUIRE(compareColumnName(virtualTable.find_column_name(&dbstat::hidden::schema), "schema"));
        }
        SECTION("storage") {
            auto storage = make_storage("", make_dbstat_table());
            storage.sync_schema();
            // eponymous virtual tables must not get created
            REQUIRE_FALSE(storage.table_exists("dbstat"));

            auto dbstatRows = storage.get_all<dbstat>();
            REQUIRE(dbstatRows.size() == 0);

            dbstatRows = storage.get_all<dbstat>(where(column<dbstat>(&dbstat::hidden::schema) == "main"));
            REQUIRE(dbstatRows.size() == 0);
        }
    }

    SECTION("virtual table instance") {
        struct mystat : dbstat {
            // A clever way of defining and using explicit column pointers for hidden `dbstat` member fields
            using hidden = dbstat::hidden_columns_for<mystat>;
        };

        SECTION("definition") {
            auto virtualTable = make_virtual_table<mystat>("mystat", using_dbstat());
            REQUIRE(compareColumnName(virtualTable.find_column_name(&dbstat::name), "name"));
            REQUIRE(compareColumnName(virtualTable.find_column_name(&dbstat::pgsize), "pgsize"));
            REQUIRE(compareColumnName(virtualTable.find_column_name(&dbstat::hidden::schema), "schema"));
        }
        SECTION("storage") {
            auto storage = make_storage("", make_virtual_table<mystat>("mystat", using_dbstat()));
            storage.sync_schema();
            storage.sync_schema_simulate();
            REQUIRE(storage.table_exists("mystat"));
            REQUIRE_FALSE(storage.table_exists("dbstat"));

            auto mystatRows = storage.get_all<mystat>();
            REQUIRE(mystatRows.size() == 1);

            mystatRows = storage.get_all<mystat>(where(column<mystat>(&dbstat::hidden::schema) == "main"));
            REQUIRE(mystatRows.size() == 1);

            mystatRows = storage.get_all<mystat>(where(mystat::hidden::schema_column == "main"));
            REQUIRE(mystatRows.size() == 1);
        }
    }
}
#endif

#ifdef SQLITE_ENABLE_RTREE
TEST_CASE("rtree virtual table schema") {
    struct DemoIndex {
        int64 id;
        float minX, maxX;
        float minY, maxY;
    };

    auto virtualTable = make_virtual_table("demo_index",
                                           using_rtree(make_column("id", &DemoIndex::id, primary_key()),
                                                       make_column("minX", &DemoIndex::minX),
                                                       make_column("maxX", &DemoIndex::maxX),
                                                       make_column("minY", &DemoIndex::minY),
                                                       make_column("maxY", &DemoIndex::maxY)));
    {
        constexpr auto compareColumnName = [](const std::string* foundValue, std::string expectedValue) {
            if (!foundValue) {
                return false;
            }
            return *foundValue == expectedValue;
        };
        REQUIRE(compareColumnName(virtualTable.find_column_name(&DemoIndex::id), "id"));
        REQUIRE(compareColumnName(virtualTable.find_column_name(&DemoIndex::maxY), "maxY"));
    }

    auto storage = make_storage("", std::move(virtualTable));
    storage.sync_schema();
    storage.sync_schema_simulate();
    REQUIRE(storage.table_exists("demo_index"));

    storage.insert(into<DemoIndex>(),
                   columns(&DemoIndex::id, &DemoIndex::minX, &DemoIndex::maxX, &DemoIndex::minY, &DemoIndex::maxY),
                   values(std::tuple(28269, -80.851471, -80.735718, 35.272560, 35.407925)));

    auto rows = storage.select(&DemoIndex::id, where(c(&DemoIndex::id) == 28269));
    REQUIRE(rows == std::vector<int64>{28269});
}
#endif
