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
        STATIC_REQUIRE_FALSE(internal::is_over_v<decltype(partition_by(&User::id))>);
        STATIC_REQUIRE_FALSE(internal::is_partition_by_v<int>);
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
    //  the frame and PARTITION BY asserts being over-strict.
    SECTION("gated factories accept their whole legal envelope") {
        std::ignore = partition_by(&User::id, add(&User::id, 1));
        std::ignore = rows(unbounded_preceding(), current_row());
        std::ignore = range(preceding(1), following(1));
        std::ignore = groups(current_row(), unbounded_following());
    }
}
