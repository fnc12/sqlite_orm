#include <type_traits>
#include <utility>
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

struct User {
    int id = 0;
    std::string name;
};

TEST_CASE("iterator_t") {
    using storage = decltype(make_storage(
        "aPath",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name))));
    using iter = decltype(std::declval<storage>().iterate<User>().begin());

    // weakly_incrementable
    static_assert(std::is_default_constructible_v<iter>, "needs to be default constructible");
    static_assert(std::is_same_v<typename iter::difference_type, std::ptrdiff_t>, "needs to have difference_type");
    static_assert(std::is_same_v<decltype(++std::declval<iter>()), iter&>, "needs to be incrementable");
    using check = decltype(std::declval<iter>()++);

    // indirectly_readable
    static_assert(std::is_same_v<decltype(*std::declval<const iter>()), const User&>,
                  "needs to be const dereferencable");

    // input_iterator
    static_assert(std::is_same_v<typename iter::iterator_category, std::input_iterator_tag>,
                  "needs to have iterator_category");

    // sentinel
    static_assert(std::is_same_v<decltype(std::declval<const iter>() == std::declval<const iter>()), bool>,
                  "supports equality checking");
}
