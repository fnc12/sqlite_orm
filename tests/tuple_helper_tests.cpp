#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <typeindex>  //  std::type_index
#include <string>  //  std::string
#include <type_traits>  //  std::decay

using namespace sqlite_orm;

TEST_CASE("tuple_helper") {
    using namespace internal;

    std::vector<std::type_index> expected;
    std::vector<std::type_index> types;
    SECTION("iterate_tuple with tuple instance") {
        auto lambda = [&types](const auto &item) {
            types.push_back(typeid(item));
        };
        SECTION("empty") {
            std::tuple<> tuple;
            iterate_tuple(tuple, lambda);
        }
        SECTION("int") {
            std::tuple<int> tuple;
            expected.push_back(typeid(int));
            iterate_tuple(tuple, lambda);
        }
        SECTION("char, long") {
            std::tuple<char, long> tuple;
            expected.push_back(typeid(char));
            expected.push_back(typeid(long));
            iterate_tuple(tuple, lambda);
        }
        SECTION("std::string, std::string, int") {
            std::tuple<std::string, std::string, int> tuple;
            expected.push_back(typeid(std::string));
            expected.push_back(typeid(std::string));
            expected.push_back(typeid(int));
            iterate_tuple(tuple, lambda);
        }
    }
    SECTION("iterate_tuple with no tuple instance") {
        auto lambda = [&types](auto *itemPointer) {
            using Item = typename std::remove_pointer<decltype(itemPointer)>::type;
            types.push_back(typeid(Item));
        };
        SECTION("empty") {
            iterate_tuple<std::tuple<>>(lambda);
        }
        SECTION("int") {
            iterate_tuple<std::tuple<int>>(lambda);
            expected.push_back(typeid(int));
        }
        SECTION("char, long") {
            iterate_tuple<std::tuple<char, long>>(lambda);
            expected.push_back(typeid(char));
            expected.push_back(typeid(long));
        }
        SECTION("std::string, std::string, int") {
            iterate_tuple<std::tuple<std::string, std::string, int>>(lambda);
            expected.push_back(typeid(std::string));
            expected.push_back(typeid(std::string));
            expected.push_back(typeid(int));
        }
    }
    REQUIRE(expected == types);
}
