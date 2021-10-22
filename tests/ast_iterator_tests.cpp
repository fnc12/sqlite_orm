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
    SECTION("excluded") {
        auto node = excluded(&User::id);
        expected.push_back(typeid(&User::id));
        internal::iterate_ast(node, lambda);
    }
    SECTION("upsert_clause") {
        auto node = on_conflict(&User::id).do_update(set(c(&User::name) = excluded(&User::name)));
        expected.push_back(typeid(&User::name));
        expected.push_back(typeid(&User::name));
        internal::iterate_ast(node, lambda);
    }
    SECTION("into") {
        auto node = into<User>();
        internal::iterate_ast(node, lambda);
    }
    SECTION("replace") {
        auto node =
            replace(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, std::string("Ellie"))));
        expected.push_back(typeid(&User::id));
        expected.push_back(typeid(&User::name));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        internal::iterate_ast(node, lambda);
    }
    SECTION("insert") {
        auto node =
            insert(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, std::string("Ellie"))));
        expected.push_back(typeid(&User::id));
        expected.push_back(typeid(&User::name));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        internal::iterate_ast(node, lambda);
    }
    SECTION("values") {
        auto node = values(std::make_tuple(1, std::string("hi")));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        internal::iterate_ast(node, lambda);
    }
    SECTION("tuple") {
        auto node = std::make_tuple(1, std::string("hi"));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        internal::iterate_ast(node, lambda);
    }
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
