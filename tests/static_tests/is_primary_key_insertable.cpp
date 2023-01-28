#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::std::decay

using namespace sqlite_orm;

TEST_CASE("is_primary_key_insertable") {
    struct User {
        int id;
        std::string username;
        std::string password;
        bool isActive;
    };

    auto insertable = std::make_tuple(  ///
        make_column("", &User::id, primary_key()),
        make_column("", &User::username, primary_key(), default_value("Clint Eastwood")),
        make_column("", &User::username, primary_key(), default_value(std::vector<int>{})),
        make_column("", &User::username, primary_key().autoincrement()));

    auto noninsertable = std::make_tuple(  ///
        make_column("", &User::username, primary_key()),
        make_column("", &User::password, primary_key()));

    iterate_tuple(insertable, [](auto& v) {
        STATIC_REQUIRE(internal::is_primary_key_insertable<std::decay_t<decltype(v)>>::value);
    });

    iterate_tuple(noninsertable, [](auto& v) {
        STATIC_REQUIRE(!internal::is_primary_key_insertable<std::decay_t<decltype(v)>>::value);
    });
}