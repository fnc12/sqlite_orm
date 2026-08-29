#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include <type_traits>  //  std::is_same
#include <memory>  //  std::unique_ptr, std::shared_ptr
#include <string>  //  std::string
#include <optional>  //  std::optional

using namespace sqlite_orm;

TEST_CASE("get* and remove accept bindable pk types") {
    struct User {
        int id = 0;
        std::string name;
    };

    using Storage = decltype(make_storage(
        "",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name))));

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<Storage>().get<User>(std::declval<int>())), User>);

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<Storage>().get_pointer<User>(std::declval<int>())),
                                  std::unique_ptr<User>>);

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<Storage>().get_no_throw<User>(std::declval<int>())),
                                  std::shared_ptr<User>>);

    STATIC_REQUIRE(
        std::is_same_v<decltype(std::declval<Storage>().get_optional<User>(std::declval<int>())), std::optional<User>>);

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<Storage>().remove<User>(std::declval<int>())), void>);
}
