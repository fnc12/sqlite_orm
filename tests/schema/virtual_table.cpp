#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000
using namespace sqlite_orm;

TEST_CASE("fts5 virtual table schema") {
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
        }
        SECTION("storage") {
            auto storage = make_storage("", make_dbstat_table());
            storage.sync_schema();
            // eponymous virtual tables must not get created
            REQUIRE_FALSE(storage.table_exists("dbstat"));

            auto dbstatRows = storage.get_all<dbstat>();
            REQUIRE(dbstatRows.size() == 0);
        }
    }

    SECTION("virtual table instance") {
        struct mystat : sqlite_orm::dbstat {};

        SECTION("definition") {
            auto virtualTable = make_virtual_table<mystat>("mystat", using_dbstat());
            REQUIRE(compareColumnName(virtualTable.find_column_name(&mystat::name), "name"));
            REQUIRE(compareColumnName(virtualTable.find_column_name(&mystat::pgsize), "pgsize"));
        }
        SECTION("storage") {
            auto storage =
                make_storage("", make_sqlite_schema_table(), make_virtual_table<mystat>("mystat", using_dbstat()));
            storage.sync_schema();
            storage.sync_schema_simulate();
            REQUIRE(storage.table_exists("mystat"));
            REQUIRE_FALSE(storage.table_exists("dbstat"));

            auto mystatRows = storage.get_all<mystat>();
            REQUIRE(mystatRows.size() == 1);
        }
    }
}
#endif

#ifdef SQLITE_ENABLE_RTREE
TEST_CASE("rtree virtual table schema") {
    struct DemoIndex {
        int64 id;
        double minX, maxX;
        double minY, maxY;
    };

    auto virtualTable = make_virtual_table("demo_index",
                                           using_rtree(make_column("id", &DemoIndex::id),
                                                       make_column("minX", &DemoIndex::minX),
                                                       make_column("maxX", &DemoIndex::maxX),
                                                       make_column("minY", &DemoIndex::minY),
                                                       make_column("maxY", &DemoIndex::maxY)));
    {
        const auto compareColumnName = [](const std::string* foundValue, std::string expectedValue) {
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
                   values(std::tuple(28215, -80.781227, -80.604706, 35.208813, 35.297367),
                          std::tuple(28216, -80.957283, -80.840599, 35.235920, 35.367825),
                          std::tuple(28217, -80.960869, -80.869431, 35.133682, 35.208233),
                          std::tuple(28226, -80.878983, -80.778275, 35.060287, 35.154446),
                          std::tuple(28227, -80.745544, -80.555382, 35.130215, 35.236916),
                          std::tuple(28244, -80.844208, -80.841988, 35.223728, 35.225471),
                          std::tuple(28262, -80.809074, -80.682938, 35.276207, 35.377747),
                          std::tuple(28269, -80.851471, -80.735718, 35.272560, 35.407925),
                          std::tuple(28270, -80.794983, -80.728966, 35.059872, 35.161823),
                          std::tuple(28273, -80.994766, -80.875259, 35.074734, 35.172836),
                          std::tuple(28277, -80.876793, -80.767586, 35.001709, 35.101063),
                          std::tuple(28278, -81.058029, -80.956375, 35.044701, 35.223812),
                          std::tuple(28280, -80.844208, -80.841972, 35.225468, 35.227203),
                          std::tuple(28282, -80.846382, -80.844193, 35.223972, 35.225655)));

    auto rows = storage.select(&DemoIndex::id,
                               where(c(&DemoIndex::minX) <= -80.77470 and c(&DemoIndex::maxX) >= -80.77470 and
                                     c(&DemoIndex::minY) <= 35.37785 and c(&DemoIndex::maxY) >= 35.37785));
    REQUIRE(rows == std::vector<int64>{28269});
}
#endif
