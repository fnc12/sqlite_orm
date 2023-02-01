#include <type_traits>
#include <utility>
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };
}

TEST_CASE("iterator_t") {
    using storage = decltype(make_storage(
        "aPath",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name))));
    using iter = decltype(std::declval<storage>().iterate<User>().begin());

    // weakly_incrementable
    STATIC_REQUIRE(std::is_default_constructible<iter>::value);
    STATIC_REQUIRE(std::is_same<typename iter::difference_type, std::ptrdiff_t>::value);
    STATIC_REQUIRE(std::is_same<decltype(++std::declval<iter>()), iter&>::value);
    using check = decltype(std::declval<iter>()++);

    // indirectly_readable
    STATIC_REQUIRE(std::is_same<decltype(*std::declval<const iter>()), const User&>::value);

    // input_iterator
    STATIC_REQUIRE(std::is_same<iter::iterator_category, std::input_iterator_tag>::value);

    // sentinel (equality comparable)
    STATIC_REQUIRE(std::is_same<decltype(std::declval<const iter>() == std::declval<const iter>()), bool>::value);
}
