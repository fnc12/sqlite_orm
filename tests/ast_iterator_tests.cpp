#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("ast_iterator") {
    struct User {
        int id = 0;
        std::string name;
    };
    SECTION("in") {
        std::vector<std::type_index> typeIndexes;
        decltype(typeIndexes) expected;
        SECTION("static") {
            auto node = c(&User::id).in(1, 2, 3);
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            internal::iterate_ast(node, [&typeIndexes](auto &value) {
                typeIndexes.push_back(typeid(value));
            });
        }
        SECTION("dynamic") {
            auto node = in(&User::id, {1, 2, 3});
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            internal::iterate_ast(node, [&typeIndexes](auto &value) {
                typeIndexes.push_back(typeid(value));
            });
        }
        REQUIRE(typeIndexes == expected);
    }
}
