#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::count_tuple, internal::is_pkcol_implicitly_insertable;

TEST_CASE("is_pkcol_implicitly_insertable") {
    struct User {
        int intId;
        int64 int64Id;
        long long longlongId;
        bool bool_;
        std::string username;
    };

    std::tuple columns(
        ///
        /// implicitly insertable
        ///
        //  works but must manually ensure data precision integrity, not yet deprecated
        make_column("", &User::intId, primary_key()),
        //  works but deprecated
        make_column("", &User::intId, primary_key().autoincrement()),
        make_column("", &User::int64Id, primary_key()),
        make_column("", &User::int64Id, primary_key().autoincrement()),
        make_column("", &User::longlongId, primary_key()),
        make_column("", &User::longlongId, primary_key().autoincrement()),
        //  works but must manually ensure data precision integrity, not yet deprecated
        make_column("", &User::bool_, primary_key()),
        make_column("", &User::username, primary_key(), default_value("Clint Eastwood")),
        make_column("", &User::username, primary_key(), default_value(std::vector<int>{})),
        ///
        /// not implicitly insertable
        ///
        make_column("", &User::username, primary_key()));

    STATIC_REQUIRE(count_tuple<decltype(columns), is_pkcol_implicitly_insertable>::value == 9);
}