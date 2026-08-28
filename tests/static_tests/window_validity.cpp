#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <tuple>  //  std::ignore

using namespace sqlite_orm;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };
}

TEST_CASE("window function arguments are validated at compile time") {
    //  `is_over` and `is_partition_by` have no consumer yet; these checks are what keeps them honest
    SECTION("window function nodes are classified") {
        STATIC_REQUIRE(internal::is_over_v<decltype(row_number().over(order_by(&User::id)))>);
        STATIC_REQUIRE(internal::is_partition_by_v<decltype(partition_by(&User::id))>);
        STATIC_REQUIRE(internal::is_window_defn_v<decltype(window("w", order_by(&User::id)))>);
        STATIC_REQUIRE(internal::is_window_ref_v<decltype(window_ref("w"))>);
        STATIC_REQUIRE(internal::is_frame_spec_v<decltype(rows(unbounded_preceding(), current_row()))>);
        STATIC_REQUIRE_FALSE(internal::is_over_v<decltype(partition_by(&User::id))>);
        STATIC_REQUIRE_FALSE(internal::is_partition_by_v<int>);
        STATIC_REQUIRE_FALSE(internal::is_window_ref_v<decltype(window("w", order_by(&User::id)))>);
    }
    SECTION("window definition elements are recognized") {
        STATIC_REQUIRE(internal::is_window_defn_element_v<decltype(partition_by(&User::id))>);
        STATIC_REQUIRE(internal::is_window_defn_element_v<decltype(order_by(&User::id))>);
        STATIC_REQUIRE(
            internal::is_window_defn_element_v<decltype(multi_order_by(order_by(&User::id), order_by(&User::name)))>);
        STATIC_REQUIRE(internal::is_window_defn_element_v<decltype(groups(current_row(), unbounded_following()))>);
        //  a named window is referenced, not defined, by `OVER name`
        STATIC_REQUIRE_FALSE(internal::is_window_defn_element_v<decltype(window_ref("w"))>);
        STATIC_REQUIRE_FALSE(internal::is_window_defn_element_v<int>);
    }
    //  every over() gates on this pack check
    SECTION("an OVER clause takes a lone window reference, or window definition elements") {
        STATIC_REQUIRE(internal::are_valid_over_arguments_v<>);
        STATIC_REQUIRE(internal::are_valid_over_arguments_v<decltype(window_ref("w"))>);
        STATIC_REQUIRE(internal::are_valid_over_arguments_v<decltype(partition_by(&User::id))>);
        STATIC_REQUIRE(internal::are_valid_over_arguments_v<decltype(order_by(&User::id)),
                                                            decltype(rows(unbounded_preceding(), current_row()))>);
        //  a window reference is admissible only on its own - the base-window-name form has no DSL spelling
        STATIC_REQUIRE_FALSE(
            internal::are_valid_over_arguments_v<decltype(window_ref("w")), decltype(order_by(&User::id))>);
        STATIC_REQUIRE_FALSE(internal::are_valid_over_arguments_v<int>);
        STATIC_REQUIRE_FALSE(internal::are_valid_over_arguments_v<decltype(where(c(&User::id) > 0))>);
    }
    SECTION("frame boundary nodes are classified") {
        STATIC_REQUIRE(internal::is_unbounded_preceding_v<decltype(unbounded_preceding())>);
        STATIC_REQUIRE(internal::is_preceding_v<decltype(preceding(1))>);
        STATIC_REQUIRE(internal::is_current_row_v<decltype(current_row())>);
        STATIC_REQUIRE(internal::is_following_v<decltype(following(1))>);
        STATIC_REQUIRE(internal::is_unbounded_following_v<decltype(unbounded_following())>);
        STATIC_REQUIRE_FALSE(internal::is_preceding_v<decltype(following(1))>);
        STATIC_REQUIRE_FALSE(internal::is_current_row_v<int>);
    }
    //  the frame factories gate on these predicates
    SECTION("frame boundaries keep their grammar positions") {
        STATIC_REQUIRE(internal::is_frame_start_bound_v<internal::unbounded_preceding_t>);
        STATIC_REQUIRE(internal::is_frame_start_bound_v<decltype(preceding(1))>);
        STATIC_REQUIRE(internal::is_frame_start_bound_v<internal::current_row_t>);
        STATIC_REQUIRE(internal::is_frame_start_bound_v<decltype(following(1))>);
        STATIC_REQUIRE_FALSE(internal::is_frame_start_bound_v<internal::unbounded_following_t>);
        STATIC_REQUIRE(internal::is_frame_end_bound_v<internal::unbounded_following_t>);
        STATIC_REQUIRE(internal::is_frame_end_bound_v<decltype(preceding(1))>);
        STATIC_REQUIRE_FALSE(internal::is_frame_end_bound_v<internal::unbounded_preceding_t>);
        STATIC_REQUIRE_FALSE(internal::is_frame_end_bound_v<int>);
    }
    //  These constructions have to keep compiling: they are the regression guard against
    //  the frame, PARTITION BY, OVER and window definition asserts being over-strict.
    SECTION("gated factories accept their whole legal envelope") {
        std::ignore = partition_by(&User::id, add(&User::id, 1));
        std::ignore = rows(unbounded_preceding(), current_row());
        std::ignore = range(preceding(1), following(1));
        std::ignore = groups(current_row(), unbounded_following());
        //  `.desc()` yields an order_by_t again, as the examples rely on
        std::ignore = window("w",
                             partition_by(&User::id),
                             order_by(&User::name).desc(),
                             groups(current_row(), unbounded_following()));
        std::ignore = window("empty");
        //  one per header the OVER gate was added to: window_functions.h, ast/rank.h, core_functions.h
        std::ignore = row_number().over(window_ref("w"));
        std::ignore = rank().over(partition_by(&User::id), order_by(&User::name));
        std::ignore = count(&User::id).over();
        std::ignore = group_concat(&User::name, std::string("."))
                          .over(partition_by(&User::id), order_by(&User::id), rows(preceding(1), following(1)));
    }
}
