#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("expression classification") {
    struct User {
        int64 id;
        std::string name;
    };

    SECTION("grammar classification") {
        STATIC_REQUIRE(internal::is_between_v<decltype(between(&User::id, 1, 2))>);
        STATIC_REQUIRE(internal::is_in_v<decltype(c(&User::id).in(1, 2))>);
        STATIC_REQUIRE(internal::is_dynamic_in_v<decltype(in(&User::id, {1, 2}))>);
        STATIC_REQUIRE(internal::is_dynamic_in_v<decltype(in(&User::id, select(&User::id)))>);
        STATIC_REQUIRE(internal::is_is_null_v<decltype(is_null(&User::name))>);
        STATIC_REQUIRE(internal::is_is_not_null_v<decltype(is_not_null(&User::name))>);
        STATIC_REQUIRE(internal::is_exists_v<decltype(exists(select(&User::id)))>);
        STATIC_REQUIRE(internal::is_cast_v<decltype(cast<std::string>(&User::id))>);
        STATIC_REQUIRE(internal::is_excluded_v<decltype(excluded(&User::name))>);
        STATIC_REQUIRE(internal::is_match_v<decltype(match(&User::name, "x"))>);
        STATIC_REQUIRE(internal::is_match_with_table_v<internal::match_with_table_t<User, std::string>>);
        STATIC_REQUIRE(internal::is_current_time_v<decltype(current_time())>);
        STATIC_REQUIRE(internal::is_current_date_v<decltype(current_date())>);
        STATIC_REQUIRE(internal::is_current_timestamp_v<decltype(current_timestamp())>);

        //  the spellings of one SQL production stay distinguishable ...
        STATIC_REQUIRE_FALSE(internal::is_in_v<decltype(in(&User::id, {1, 2}))>);
        STATIC_REQUIRE_FALSE(internal::is_dynamic_in_v<decltype(c(&User::id).in(1, 2))>);
        STATIC_REQUIRE_FALSE(internal::is_match_v<internal::match_with_table_t<User, std::string>>);
        STATIC_REQUIRE_FALSE(internal::is_match_with_table_v<decltype(match(&User::name, "x"))>);
        //  ... but are the one IN production
        STATIC_REQUIRE(internal::is_any_in_v<decltype(c(&User::id).in(1, 2))>);
        STATIC_REQUIRE(internal::is_any_in_v<decltype(in(&User::id, {1, 2}))>);

        //  the NULL tests are distinct productions
        STATIC_REQUIRE_FALSE(internal::is_is_null_v<decltype(is_not_null(&User::name))>);
        STATIC_REQUIRE_FALSE(internal::is_is_not_null_v<decltype(is_null(&User::name))>);
        STATIC_REQUIRE_FALSE(internal::is_current_time_v<decltype(current_date())>);
        STATIC_REQUIRE_FALSE(internal::is_current_date_v<decltype(current_timestamp())>);
        STATIC_REQUIRE_FALSE(internal::is_current_timestamp_v<decltype(current_time())>);

        STATIC_REQUIRE_FALSE(internal::is_between_v<int>);
        STATIC_REQUIRE_FALSE(internal::is_any_in_v<int>);
        STATIC_REQUIRE_FALSE(internal::is_exists_v<decltype(select(&User::id))>);
        STATIC_REQUIRE_FALSE(internal::is_cast_v<decltype(&User::id)>);
        STATIC_REQUIRE_FALSE(internal::is_excluded_v<decltype(&User::name)>);
    }

    SECTION("operand classification") {
        STATIC_REQUIRE(internal::is_operator_argument_v<decltype(cast<std::string>(&User::id))>);
        STATIC_REQUIRE(internal::is_operator_argument_v<decltype(excluded(&User::name))>);
        STATIC_REQUIRE(internal::is_operator_argument_v<decltype(c(&User::id))>);
    }

    SECTION("projections") {
        using between_type = decltype(between(&User::id, 1, 2));
        STATIC_REQUIRE(std::is_same<internal::expression_type_t<between_type>, decltype(&User::id)>::value);
        STATIC_REQUIRE(std::is_same<internal::lower_type_t<between_type>, int>::value);
        STATIC_REQUIRE(std::is_same<internal::upper_type_t<between_type>, int>::value);

        using in_type = decltype(c(&User::id).in(1, 2));
        STATIC_REQUIRE(std::is_same<internal::left_type_t<in_type>, decltype(&User::id)>::value);
        STATIC_REQUIRE(std::is_same<internal::argument_type_t<in_type>, std::tuple<int, int>>::value);

        using dynamic_in_type = decltype(in(&User::id, {1, 2}));
        STATIC_REQUIRE(std::is_same<internal::left_type_t<dynamic_in_type>, decltype(&User::id)>::value);
        STATIC_REQUIRE(std::is_same<internal::argument_type_t<dynamic_in_type>, std::vector<int>>::value);

        using cast_type = decltype(cast<std::string>(&User::id));
        STATIC_REQUIRE(std::is_same<internal::to_type_t<cast_type>, std::string>::value);

        STATIC_REQUIRE(
            std::is_same<internal::mapped_type_t<internal::match_with_table_t<User, std::string>>, User>::value);
    }
}
