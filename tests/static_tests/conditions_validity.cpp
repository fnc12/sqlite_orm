#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::clause_rank_v, internal::clause_holds_no_clause_v, internal::check_clause_order_v;
using internal::is_statement_clause;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        int userId = 0;
    };
}

TEST_CASE("statement clause classification and order are computed at compile time") {
    using From = decltype(from<User>());
    using Join = decltype(cross_join<Visit>());
    using Where = decltype(where(c(&User::id) > 0));
    using GroupBy = decltype(group_by(&User::id));
    using OrderBy = decltype(order_by(&User::id));
    using Limit = decltype(limit(5));
    using Window = decltype(window("w", partition_by(&User::id)));

    SECTION("clause ranks follow the canonical clause order") {
        STATIC_REQUIRE(clause_rank_v<From> == 1);
        STATIC_REQUIRE(clause_rank_v<Join> == 2);
        STATIC_REQUIRE(clause_rank_v<Where> == 3);
        STATIC_REQUIRE(clause_rank_v<GroupBy> == 4);
        STATIC_REQUIRE(clause_rank_v<Window> == 5);
        STATIC_REQUIRE(clause_rank_v<OrderBy> == 6);
        STATIC_REQUIRE(clause_rank_v<Limit> == 7);
    }
    SECTION("expressions are not statement clauses") {
        STATIC_REQUIRE(clause_rank_v<int> == 0);
        STATIC_REQUIRE(clause_rank_v<int User::*> == 0);
        STATIC_REQUIRE(clause_rank_v<decltype(c(&User::id) > 0)> == 0);
        STATIC_REQUIRE_FALSE(is_statement_clause<decltype(count<User>())>::value);
        STATIC_REQUIRE(is_statement_clause<Where>::value);
    }
    SECTION("the canonical order is accepted") {
        STATIC_REQUIRE(check_clause_order_v<std::tuple<>>);
        STATIC_REQUIRE(check_clause_order_v<std::tuple<Where>>);
        STATIC_REQUIRE(check_clause_order_v<std::tuple<From, Join, Where, GroupBy, Window, OrderBy, Limit>>);
        STATIC_REQUIRE(check_clause_order_v<std::tuple<Join, Join, Where>>);
    }
    SECTION("a wrong order is rejected") {
        STATIC_REQUIRE_FALSE(check_clause_order_v<std::tuple<GroupBy, Where>>);
        STATIC_REQUIRE_FALSE(check_clause_order_v<std::tuple<Limit, OrderBy>>);
        STATIC_REQUIRE_FALSE(check_clause_order_v<std::tuple<OrderBy, Window>>);
        STATIC_REQUIRE_FALSE(check_clause_order_v<std::tuple<Where, Join>>);
        STATIC_REQUIRE_FALSE(check_clause_order_v<std::tuple<OrderBy, From>>);
    }
    SECTION("clauses holding expressions are valid") {
        STATIC_REQUIRE(clause_holds_no_clause_v<Where>);
        STATIC_REQUIRE(clause_holds_no_clause_v<GroupBy>);
        STATIC_REQUIRE(clause_holds_no_clause_v<OrderBy>);
        STATIC_REQUIRE(clause_holds_no_clause_v<Limit>);
        STATIC_REQUIRE(clause_holds_no_clause_v<decltype(limit(select(count<User>())))>);
        STATIC_REQUIRE(clause_holds_no_clause_v<decltype(multi_order_by(order_by(&User::id), order_by(&User::name)))>);
        STATIC_REQUIRE(clause_holds_no_clause_v<decltype(group_by(&User::id).having(c(&User::id) > 0))>);
    }
    SECTION("clauses holding clauses are rejected") {
        STATIC_REQUIRE_FALSE(clause_holds_no_clause_v<decltype(where(where(c(&User::id) > 0)))>);
        STATIC_REQUIRE_FALSE(clause_holds_no_clause_v<decltype(where(group_by(&User::id)))>);
        STATIC_REQUIRE_FALSE(clause_holds_no_clause_v<decltype(order_by(limit(5)))>);
        STATIC_REQUIRE_FALSE(clause_holds_no_clause_v<decltype(group_by(where(c(&User::id) > 0)))>);
        STATIC_REQUIRE_FALSE(clause_holds_no_clause_v<decltype(limit(where(c(&User::id) > 0)))>);
        STATIC_REQUIRE_FALSE(
            clause_holds_no_clause_v<decltype(multi_order_by(order_by(&User::id), where(c(&User::id) > 0)))>);
        STATIC_REQUIRE_FALSE(
            clause_holds_no_clause_v<decltype(multi_order_by(order_by(&User::id), order_by(limit(5))))>);
    }
}
