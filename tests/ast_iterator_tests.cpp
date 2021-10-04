#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("ast_iterator") {
    struct User {
        int id = 0;
        std::string name;
    };
    std::vector<std::type_index> typeIndexes;
    decltype(typeIndexes) expected;
    auto lambda = [&typeIndexes](auto &value) {
        typeIndexes.push_back(typeid(value));
    };
    SECTION("in") {
        SECTION("static") {
            auto node = c(&User::id).in(1, 2, 3);
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            internal::iterate_ast(node, lambda);
        }
        SECTION("dynamic") {
            auto node = in(&User::id, {1, 2, 3});
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            internal::iterate_ast(node, lambda);
        }
    }
    SECTION("function_call") {
        struct Func {
            bool operator()(int value) const {
                return value % 2 == 0;
            }
        };
        auto node = func<Func>(&User::id);
        expected.push_back(typeid(&User::id));
        internal::iterate_ast(node, lambda);
    }
    REQUIRE(typeIndexes == expected);
}
