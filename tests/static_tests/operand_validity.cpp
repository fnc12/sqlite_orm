#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <functional>  //  std::reference_wrapper
#include <tuple>  //  std::tuple
#include <vector>  //  std::vector

using namespace sqlite_orm;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };
}

TEST_CASE("named factory operands are validated at compile time") {
    SECTION("recognized operands and bindable values are accepted") {
        STATIC_REQUIRE(internal::is_operand_or_bindable<int User::*>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(c(&User::id))>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(column<User>(&User::id))>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<int>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<const char*>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<std::reference_wrapper<int>>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<std::tuple<int, int>>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(select(&User::id))>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(length(&User::name))>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(cast<int>(&User::name))>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(new_(&User::id))>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(old(&User::id))>::value);
        STATIC_REQUIRE(internal::is_operand_or_bindable<decltype(excluded(&User::id))>::value);
    }
    SECTION("garbage types and statement clauses are rejected") {
        STATIC_REQUIRE_FALSE(internal::is_operand_or_bindable<User>::value);
        STATIC_REQUIRE_FALSE(internal::is_operand_or_bindable<std::vector<int>>::value);
        STATIC_REQUIRE_FALSE(internal::is_operand_or_bindable<decltype(where(c(&User::id) > 0))>::value);
        STATIC_REQUIRE_FALSE(internal::is_operand_or_bindable<decltype(group_by(&User::id))>::value);
        STATIC_REQUIRE_FALSE(internal::is_operand_or_bindable<decltype(limit(5))>::value);
    }
    SECTION("both operands are checked") {
        STATIC_REQUIRE(internal::are_valid_operands<int User::*, int>::value);
        STATIC_REQUIRE(internal::are_valid_operands<int, int>::value);
        STATIC_REQUIRE_FALSE(internal::are_valid_operands<int User::*, User>::value);
        STATIC_REQUIRE_FALSE(internal::are_valid_operands<User, int>::value);
    }
    SECTION("an assignment target must be referencable") {
        STATIC_REQUIRE(internal::is_referencable_operand<int User::*>::value);
        STATIC_REQUIRE(internal::is_referencable_operand<decltype(column<User>(&User::id))>::value);
        STATIC_REQUIRE_FALSE(internal::is_referencable_operand<int>::value);
        STATIC_REQUIRE_FALSE(internal::is_referencable_operand<const char*>::value);
    }
}
