#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::is_same

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

    SECTION("conditions") {
        STATIC_REQUIRE(internal::is_like_v<decltype(like(&User::name, "a%"))>);
        STATIC_REQUIRE(internal::is_like_v<decltype(like(&User::name, "a%", "\\"))>);
        STATIC_REQUIRE(internal::is_glob_v<decltype(glob(&User::name, "a*"))>);
        STATIC_REQUIRE_FALSE(internal::is_glob_v<decltype(like(&User::name, "a%"))>);
        STATIC_REQUIRE(internal::is_negated_condition_v<decltype(!is_null(&User::name))>);
        STATIC_REQUIRE_FALSE(internal::is_negated_condition_v<decltype(is_null(&User::name))>);
        STATIC_REQUIRE(internal::is_collate_v<decltype(is_equal(&User::name, "a").collate_nocase())>);
        STATIC_REQUIRE(internal::is_named_collate_v<decltype(is_equal(&User::name, "a").collate("custom"))>);
        STATIC_REQUIRE_FALSE(internal::is_collate_v<decltype(is_equal(&User::name, "a").collate("custom"))>);
        STATIC_REQUIRE(internal::is_equal_with_table_v<internal::is_equal_with_table_t<User, std::string>>);
        STATIC_REQUIRE_FALSE(internal::is_equal_with_table_v<decltype(is_equal(&User::name, "a"))>);
        STATIC_REQUIRE_FALSE(internal::is_binary_condition_v<internal::is_equal_with_table_t<User, std::string>>);

        //  a collated condition is walked into like any other node, both by the AST iterator and the node tuple
        using Collated = decltype(is_equal(&User::id, 1).collate_binary());
        STATIC_REQUIRE(std::is_same<internal::node_tuple_t<Collated>, std::tuple<decltype(&User::id), int>>::value);
        using LikeEscape = decltype(like(&User::name, std::string("a%"), std::string("\\")));
        STATIC_REQUIRE(std::is_same<internal::node_tuple_t<LikeEscape>,
                                    std::tuple<decltype(&User::name), std::string, std::string>>::value);
    }

    SECTION("joins") {
        //  every join is the one join-operator production ...
        STATIC_REQUIRE(internal::is_any_join_v<decltype(cross_join<User>())>);
        STATIC_REQUIRE(internal::is_any_join_v<decltype(natural_join<User>())>);
        STATIC_REQUIRE(internal::is_any_join_v<decltype(join<User>(on(is_equal(&User::id, 1))))>);
        STATIC_REQUIRE(internal::is_any_join_v<decltype(left_join<User>(on(is_equal(&User::id, 1))))>);
        STATIC_REQUIRE(internal::is_any_join_v<decltype(left_outer_join<User>(using_(&User::id)))>);
        STATIC_REQUIRE(internal::is_any_join_v<decltype(inner_join<User>(using_(&User::id)))>);
        STATIC_REQUIRE_FALSE(internal::is_any_join_v<decltype(on(is_equal(&User::id, 1)))>);
        STATIC_REQUIRE_FALSE(internal::is_any_join_v<decltype(from<User>())>);
        //  ... telling the constrained ones apart by their constraint
        STATIC_REQUIRE(polyfill::is_detected_v<internal::on_type_t, decltype(inner_join<User>(using_(&User::id)))>);
        STATIC_REQUIRE_FALSE(polyfill::is_detected_v<internal::on_type_t, decltype(cross_join<User>())>);

        STATIC_REQUIRE(internal::is_on_v<decltype(on(is_equal(&User::id, 1)))>);
        STATIC_REQUIRE(internal::is_using_v<decltype(using_(&User::id))>);
        STATIC_REQUIRE_FALSE(internal::is_on_v<decltype(using_(&User::id))>);
        STATIC_REQUIRE_FALSE(internal::is_using_v<decltype(on(is_equal(&User::id, 1)))>);

        using Join = decltype(join<User>(on(is_equal(&User::id, 1))));
        STATIC_REQUIRE(std::is_same<internal::node_tuple_t<Join>, std::tuple<decltype(&User::id), int>>::value);
    }

    SECTION("rowid") {
        //  every spelling, unqualified or qualified by the table, is the one rowid reference ...
        STATIC_REQUIRE(internal::is_any_rowid_v<decltype(rowid())>);
        STATIC_REQUIRE(internal::is_any_rowid_v<decltype(oid())>);
        STATIC_REQUIRE(internal::is_any_rowid_v<decltype(_rowid_())>);
        STATIC_REQUIRE(internal::is_any_rowid_v<decltype(rowid<User>())>);
        STATIC_REQUIRE(internal::is_any_rowid_v<decltype(oid<User>())>);
        STATIC_REQUIRE(internal::is_any_rowid_v<decltype(_rowid_<User>())>);
        STATIC_REQUIRE_FALSE(internal::is_any_rowid_v<decltype(&User::id)>);
        STATIC_REQUIRE_FALSE(internal::is_any_rowid_v<int>);
        //  a tuple inherits its empty element types, yet is not a rowid
        STATIC_REQUIRE_FALSE(internal::is_any_rowid_v<std::tuple<internal::rowid_t, internal::oid_t>>);
        //  ... qualified ones carry the table as their type
        STATIC_REQUIRE(std::is_same<internal::type_t<decltype(oid<User>())>, User>::value);
        STATIC_REQUIRE_FALSE(polyfill::is_detected_v<internal::type_t, decltype(oid())>);
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
