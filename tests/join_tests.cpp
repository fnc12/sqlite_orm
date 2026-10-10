#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <algorithm>  //  std::sort
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <vector>  //  std::vector

using namespace sqlite_orm;

/*
 *  A self-join of an aliased table: the joined table has to be declared with its alias, whether the join carries an
 *  explicit constraint (LEFT JOIN ... ON) or an implicit one (CROSS JOIN, NATURAL JOIN), and the table it is joined
 *  to has to stay in the FROM clause.
 */
TEST_CASE("Self-join of an aliased table") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storage = make_storage(
        "",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();
    storage.replace(User{1, "a"});
    storage.replace(User{2, "b"});

    using s = alias_s<User>;
    std::vector<std::tuple<int, int>> rows;
    std::vector<std::tuple<int, int>> expected;
    SECTION("CROSS JOIN") {
        rows = storage.select(columns(&User::id, alias_column<s>(&User::id)), cross_join<s>());
        expected = {{1, 1}, {1, 2}, {2, 1}, {2, 2}};
    }
    SECTION("NATURAL JOIN") {
        rows = storage.select(columns(&User::id, alias_column<s>(&User::id)), natural_join<s>());
        expected = {{1, 1}, {2, 2}};
    }
    SECTION("LEFT JOIN") {
        rows = storage.select(columns(&User::id, alias_column<s>(&User::id)),
                              left_join<s>(on(is_equal(alias_column<s>(&User::id), &User::id))));
        expected = {{1, 1}, {2, 2}};
    }
    std::sort(rows.begin(), rows.end());
    REQUIRE(rows == expected);
}

/*
 *  The join operators SQLite offers, with their constraint left implicit, and NATURAL combined with them.
 *  The tables share the column "id", which is what NATURAL joins match.
 */
TEST_CASE("Join operators") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        int userId = 0;
    };
    auto storage = make_storage(
        "",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
        make_table("visits", make_column("id", &Visit::id, primary_key()), make_column("user_id", &Visit::userId)));
    storage.sync_schema();
    storage.replace(User{1, "a"});
    storage.replace(User{2, "b"});
    storage.replace(Visit{1, 1});
    storage.replace(Visit{3, 2});

    size_t rowCount = 0;
    size_t expected = 0;
    SECTION("JOIN without constraint") {
        rowCount = storage.select(columns(&User::id, &Visit::id), join<Visit>()).size();
        expected = 4;
    }
    SECTION("LEFT JOIN without constraint") {
        rowCount = storage.select(columns(&User::id, &Visit::id), left_join<Visit>()).size();
        expected = 4;
    }
    SECTION("NATURAL INNER JOIN") {
        rowCount = storage.select(columns(&User::id, &Visit::id), natural_inner_join<Visit>()).size();
        expected = 1;
    }
    SECTION("NATURAL LEFT JOIN") {
        rowCount = storage.select(columns(&User::id, &Visit::id), natural_left_join<Visit>()).size();
        expected = 2;
    }
    SECTION("NATURAL LEFT OUTER JOIN") {
        rowCount = storage.select(columns(&User::id, &Visit::id), natural_left_outer_join<Visit>()).size();
        expected = 2;
    }
#if SQLITE_VERSION_NUMBER >= 3039000
    SECTION("RIGHT JOIN") {
        rowCount =
            storage.select(columns(&User::id, &Visit::id), right_join<Visit>(on(is_equal(&User::id, &Visit::userId))))
                .size();
        expected = 2;
    }
    SECTION("NATURAL RIGHT JOIN") {
        rowCount = storage.select(columns(&User::id, &Visit::id), natural_right_join<Visit>()).size();
        expected = 2;
    }
    SECTION("FULL OUTER JOIN") {
        rowCount = storage.select(columns(&User::id, &Visit::id), full_outer_join<Visit>(using_(&Visit::id))).size();
        expected = 3;
    }
    SECTION("NATURAL FULL JOIN") {
        rowCount = storage.select(columns(&User::id, &Visit::id), natural_full_join<Visit>()).size();
        expected = 3;
    }
#endif
    REQUIRE(rowCount == expected);
}
