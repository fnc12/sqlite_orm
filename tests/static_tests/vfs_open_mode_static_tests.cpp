#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("db_open_mode flag conversion returns expected flags") {
    STATIC_REQUIRE(internal::db_open_mode_to_int_flags(db_open_mode::default_) ==
                   (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::db_open_mode_to_int_flags(db_open_mode::create_readwrite) ==
                   (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::db_open_mode_to_int_flags(db_open_mode::readonly) == (SQLITE_OPEN_READONLY));
}
