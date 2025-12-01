#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using namespace internal;

TEST_CASE("is_pkcol_implicitly_insertable") {
    struct User {
        int id;
        int64 id2;
        bool b;
        std::string username;
    };

    std::tuple columns(
        ///
        make_column("", &User::id, primary_key()),
        make_column("", &User::id2, primary_key().autoincrement()),
        make_column("", &User::b, primary_key()),
        make_column("", &User::username, primary_key(), default_value("Clint Eastwood")),
        make_column("", &User::username, primary_key(), default_value(std::vector<int>{})),
        ///
        make_column("", &User::username, primary_key()));

    STATIC_REQUIRE(count_tuple<decltype(columns), is_pkcol_implicitly_insertable>::value == 5);
}