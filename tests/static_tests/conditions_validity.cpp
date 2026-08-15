#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

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

    SECTION("clause ranks follow the canonical clause order") {
        STATIC_REQUIRE(internal::clause_rank_v<From> == 1);
        STATIC_REQUIRE(internal::clause_rank_v<Join> == 2);
        STATIC_REQUIRE(internal::clause_rank_v<Where> == 3);
        STATIC_REQUIRE(internal::clause_rank_v<GroupBy> == 4);
        STATIC_REQUIRE(internal::clause_rank_v<OrderBy> == 5);
        STATIC_REQUIRE(internal::clause_rank_v<Limit> == 6);
    }
    SECTION("expressions are not statement clauses") {
        STATIC_REQUIRE(internal::clause_rank_v<int> == 0);
        STATIC_REQUIRE(internal::clause_rank_v<int User::*> == 0);
        STATIC_REQUIRE(internal::clause_rank_v<decltype(c(&User::id) > 0)> == 0);
        STATIC_REQUIRE_FALSE(internal::is_statement_clause<decltype(count<User>())>::value);
        STATIC_REQUIRE(internal::is_statement_clause<Where>::value);
    }
    SECTION("the canonical order is accepted") {
        STATIC_REQUIRE(internal::check_clause_order<std::tuple<>>::value);
        STATIC_REQUIRE(internal::check_clause_order<std::tuple<Where>>::value);
        STATIC_REQUIRE(internal::check_clause_order<std::tuple<From, Join, Where, GroupBy, OrderBy, Limit>>::value);
        STATIC_REQUIRE(internal::check_clause_order<std::tuple<Join, Join, Where>>::value);
    }
    SECTION("a wrong order is rejected") {
        STATIC_REQUIRE_FALSE(internal::check_clause_order<std::tuple<GroupBy, Where>>::value);
        STATIC_REQUIRE_FALSE(internal::check_clause_order<std::tuple<Limit, OrderBy>>::value);
        STATIC_REQUIRE_FALSE(internal::check_clause_order<std::tuple<Where, Join>>::value);
        STATIC_REQUIRE_FALSE(internal::check_clause_order<std::tuple<OrderBy, From>>::value);
    }
    SECTION("clauses holding expressions are valid") {
        STATIC_REQUIRE(internal::clause_holds_no_clause<Where>::value);
        STATIC_REQUIRE(internal::clause_holds_no_clause<GroupBy>::value);
        STATIC_REQUIRE(internal::clause_holds_no_clause<OrderBy>::value);
        STATIC_REQUIRE(internal::clause_holds_no_clause<Limit>::value);
        STATIC_REQUIRE(internal::clause_holds_no_clause<decltype(limit(select(count<User>())))>::value);
        STATIC_REQUIRE(internal::clause_holds_no_clause<decltype(multi_order_by(order_by(&User::id),
                                                                                order_by(&User::name)))>::value);
        STATIC_REQUIRE(internal::clause_holds_no_clause<decltype(group_by(&User::id).having(c(&User::id) > 0))>::value);
    }
    SECTION("clauses holding clauses are rejected") {
        STATIC_REQUIRE_FALSE(internal::clause_holds_no_clause<decltype(where(where(c(&User::id) > 0)))>::value);
        STATIC_REQUIRE_FALSE(internal::clause_holds_no_clause<decltype(where(group_by(&User::id)))>::value);
        STATIC_REQUIRE_FALSE(internal::clause_holds_no_clause<decltype(order_by(limit(5)))>::value);
        STATIC_REQUIRE_FALSE(internal::clause_holds_no_clause<decltype(group_by(where(c(&User::id) > 0)))>::value);
        STATIC_REQUIRE_FALSE(internal::clause_holds_no_clause<decltype(limit(where(c(&User::id) > 0)))>::value);
        STATIC_REQUIRE_FALSE(internal::clause_holds_no_clause<
                             decltype(multi_order_by(order_by(&User::id), where(c(&User::id) > 0)))>::value);
    }
}
