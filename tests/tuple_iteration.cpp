#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <typeindex>  //  std::type_index
#include <string>  //  std::string
#include <type_traits>  //  std::remove_pointer

using namespace sqlite_orm;

TEST_CASE("tuple iteration") {
    using namespace internal;

    std::vector<std::type_index> expected;
    std::vector<std::type_index> types;
    SECTION("iterate_tuple with tuple instance") {
        auto lambda = [&types](const auto &item) {
            types.emplace_back(typeid(item));
        };
        SECTION("empty") {
            std::tuple<> tuple;
            iterate_tuple(tuple, lambda);
        }
        SECTION("int") {
            std::tuple<int> tuple;
            iterate_tuple(tuple, lambda);
            expected = {typeid(int)};
        }
        SECTION("std::string, long") {
            std::tuple<std::string, long> tuple;
            iterate_tuple(tuple, lambda);
            expected = {typeid(std::string), typeid(long)};
        }
        SECTION("index selection") {
            constexpr size_t selectedIdx = 1;
            std::tuple<std::string, long> tuple;
            iterate_tuple(tuple, std::index_sequence<selectedIdx>{}, lambda);
            expected = {typeid(long)};
        }
    }
    SECTION("iterate_tuple with no tuple instance") {
        auto lambda = [&types](auto *itemPointer) {
            using Item = std::remove_pointer_t<decltype(itemPointer)>;
            types.emplace_back(typeid(Item));
        };
        SECTION("empty") {
            iterate_tuple<std::tuple<>>(lambda);
        }
        SECTION("int") {
            iterate_tuple<std::tuple<int>>(lambda);
            expected = {typeid(int)};
        }
        SECTION("std::string, long") {
            iterate_tuple<std::tuple<std::string, long>>(lambda);
            expected = {typeid(std::string), typeid(long)};
        }
    }
    REQUIRE(expected == types);
}

TEST_CASE("creation from tuple") {
    using namespace internal;
    using Catch::Matchers::Equals;

    std::tuple<std::string, std::string> tpl{"abc", "xyz"};
    SECTION("identity") {
        std::vector<std::string> expected{get<0>(tpl), get<1>(tpl)};
        auto strings = create_from_tuple<std::vector<std::string>>(tpl);
        REQUIRE_THAT(strings, Equals(expected));
    }
    SECTION("projected") {
        std::vector<const char *> expected{get<0>(tpl).c_str(), get<1>(tpl).c_str()};
        auto strings = create_from_tuple<std::vector<const char *>>(tpl, &std::string::c_str);
        REQUIRE_THAT(strings, Equals(expected));
    }
    SECTION("index selection") {
        constexpr size_t selectedIdx = 1;
        std::vector<const char *> expected{get<selectedIdx>(tpl).c_str()};
        auto strings =
            create_from_tuple<std::vector<const char *>>(tpl, std::index_sequence<selectedIdx>{}, &std::string::c_str);
        REQUIRE_THAT(strings, Equals(expected));
    }
}
