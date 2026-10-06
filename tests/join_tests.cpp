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
