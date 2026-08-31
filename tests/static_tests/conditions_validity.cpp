#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::is_delete_clause, internal::check_delete_clause_order_v;
using internal::is_select_clause, internal::is_statement_clause;
using internal::is_update_clause, internal::check_update_clause_order_v;
using internal::select_clause_rank_v, internal::check_select_clause_order_v;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        int userId = 0;
    };

    using From = decltype(from<User>());
    //  the table-valued-function spelling of FROM; only its type matters for the clause traits
    using From2 = internal::from2_t<User>;
    using Join = decltype(cross_join<Visit>());
    using Where = decltype(where(c(&User::id) > 0));
    using GroupBy = decltype(group_by(&User::id));
    using OrderBy = decltype(order_by(&User::id));
    using Limit = decltype(limit(5));
    using Window = decltype(window("w", partition_by(&User::id)));
}

TEST_CASE("statement clause classification and order are computed at compile time") {
    SECTION("clause ranks follow the canonical clause order") {
        STATIC_REQUIRE(select_clause_rank_v<From> == 1);
        STATIC_REQUIRE(select_clause_rank_v<From2> == 1);
        STATIC_REQUIRE(select_clause_rank_v<Join> == 2);
        STATIC_REQUIRE(select_clause_rank_v<Where> == 3);
        STATIC_REQUIRE(select_clause_rank_v<GroupBy> == 4);
        STATIC_REQUIRE(select_clause_rank_v<Window> == 5);
        STATIC_REQUIRE(select_clause_rank_v<OrderBy> == 6);
        STATIC_REQUIRE(select_clause_rank_v<Limit> == 7);
    }
    SECTION("expressions are not statement clauses") {
        STATIC_REQUIRE(select_clause_rank_v<int> == 0);
        STATIC_REQUIRE(select_clause_rank_v<int User::*> == 0);
        STATIC_REQUIRE(select_clause_rank_v<decltype(c(&User::id) > 0)> == 0);
        STATIC_REQUIRE_FALSE(is_select_clause<decltype(count<User>())>::value);
        STATIC_REQUIRE(is_select_clause<Where>::value);
    }
    SECTION("the canonical order is accepted") {
        STATIC_REQUIRE(check_select_clause_order_v<std::tuple<>>);
        STATIC_REQUIRE(check_select_clause_order_v<std::tuple<Where>>);
        STATIC_REQUIRE(check_select_clause_order_v<std::tuple<From, Join, Where, GroupBy, Window, OrderBy, Limit>>);
        STATIC_REQUIRE(check_select_clause_order_v<std::tuple<From2, Join, Where>>);
        STATIC_REQUIRE(check_select_clause_order_v<std::tuple<Join, Join, Where>>);
    }
    SECTION("a wrong order is rejected") {
        STATIC_REQUIRE_FALSE(check_select_clause_order_v<std::tuple<GroupBy, Where>>);
        STATIC_REQUIRE_FALSE(check_select_clause_order_v<std::tuple<Limit, OrderBy>>);
        STATIC_REQUIRE_FALSE(check_select_clause_order_v<std::tuple<OrderBy, Window>>);
        STATIC_REQUIRE_FALSE(check_select_clause_order_v<std::tuple<Where, Join>>);
        STATIC_REQUIRE_FALSE(check_select_clause_order_v<std::tuple<OrderBy, From>>);
    }
    //  These constructions have to keep compiling: each one passes an expression the clause factories must
    //  not mistake for a clause, so they are the regression guard against the new asserts being over-strict.
    SECTION("clauses holding expressions are accepted by their factories") {
        STATIC_REQUIRE(is_select_clause<decltype(where(c(&User::id) > 0))>::value);
        STATIC_REQUIRE(is_select_clause<decltype(group_by(&User::id))>::value);
        STATIC_REQUIRE(is_select_clause<decltype(order_by(&User::id))>::value);
        STATIC_REQUIRE(is_select_clause<decltype(limit(5))>::value);
        STATIC_REQUIRE(is_select_clause<decltype(limit(5, offset(10)))>::value);
        //  a scalar subquery is an expression, not a clause
        STATIC_REQUIRE(is_select_clause<decltype(limit(select(count<User>())))>::value);
        STATIC_REQUIRE(is_select_clause<decltype(multi_order_by(order_by(&User::id), order_by(&User::name)))>::value);
        STATIC_REQUIRE(is_select_clause<decltype(group_by(&User::id).having(c(&User::id) > 0))>::value);
    }
    //  The clause factories now reject a nested clause themselves, so a node such as `where(group_by(...))`
    //  can no longer be formed to be tested. What is asserted instead is the predicate the factories gate on,
    //  applied to legally constructed clauses - the same approach as `operand_validity.cpp`.
    SECTION("every clause kind is recognized as a statement clause") {
        STATIC_REQUIRE(is_statement_clause<From>::value);
        STATIC_REQUIRE(is_statement_clause<From2>::value);
        STATIC_REQUIRE(is_statement_clause<Join>::value);
        STATIC_REQUIRE(is_statement_clause<Where>::value);
        STATIC_REQUIRE(is_statement_clause<GroupBy>::value);
        STATIC_REQUIRE(is_statement_clause<Window>::value);
        STATIC_REQUIRE(is_statement_clause<OrderBy>::value);
        STATIC_REQUIRE(is_statement_clause<Limit>::value);
    }
    SECTION("expressions admissible as a clause payload are not statement clauses") {
        STATIC_REQUIRE_FALSE(is_statement_clause<decltype(c(&User::id) > 0)>::value);
        STATIC_REQUIRE_FALSE(is_statement_clause<int User::*>::value);
        STATIC_REQUIRE_FALSE(is_statement_clause<decltype(select(count<User>()))>::value);
        STATIC_REQUIRE_FALSE(is_statement_clause<int>::value);
    }
    //  `ordering-term` is narrower than `expr`, so multi ORDER BY gates on a positive predicate instead
    SECTION("a multi ORDER BY argument must be an ORDER BY term") {
        STATIC_REQUIRE(internal::is_order_by<OrderBy>::value);
        STATIC_REQUIRE(internal::is_order_by<decltype(order_by(&User::id).asc())>::value);
        STATIC_REQUIRE_FALSE(internal::is_order_by<Where>::value);
        STATIC_REQUIRE_FALSE(internal::is_order_by<Limit>::value);
        STATIC_REQUIRE_FALSE(internal::is_order_by<decltype(c(&User::id) > 0)>::value);
        //  an ORDER BY that is not a single ordering term cannot be nested in a multi ORDER BY
        STATIC_REQUIRE_FALSE(internal::is_order_by<decltype(multi_order_by(order_by(&User::id)))>::value);
        STATIC_REQUIRE_FALSE(internal::is_order_by<decltype(dynamic_order_by(make_storage("")))>::value);
    }
    //  the three spellings of the one ORDER BY clause production
    SECTION("every ORDER BY spelling is an ORDER BY clause") {
        STATIC_REQUIRE(internal::is_any_order_by<OrderBy>::value);
        STATIC_REQUIRE(internal::is_multi_order_by<decltype(multi_order_by(order_by(&User::id)))>::value);
        STATIC_REQUIRE(internal::is_any_order_by<decltype(multi_order_by(order_by(&User::id)))>::value);
        STATIC_REQUIRE(internal::is_dynamic_order_by<decltype(dynamic_order_by(make_storage("")))>::value);
        STATIC_REQUIRE(internal::is_any_order_by<decltype(dynamic_order_by(make_storage("")))>::value);
        STATIC_REQUIRE_FALSE(internal::is_any_order_by<Where>::value);
    }
}

TEST_CASE("the DML statements take their own subset of the clauses") {
    SECTION("a delete statement takes WHERE, ORDER BY and LIMIT") {
        STATIC_REQUIRE(is_delete_clause<Where>::value);
        STATIC_REQUIRE(is_delete_clause<OrderBy>::value);
        STATIC_REQUIRE(is_delete_clause<Limit>::value);
        STATIC_REQUIRE_FALSE(is_delete_clause<From>::value);
        STATIC_REQUIRE_FALSE(is_delete_clause<Join>::value);
        STATIC_REQUIRE_FALSE(is_delete_clause<GroupBy>::value);
        STATIC_REQUIRE_FALSE(is_delete_clause<Window>::value);
    }
    //  the FROM clause of `UPDATE ... SET ... FROM ...` and its joins exist as of SQLite 3.33.0
    SECTION("an update statement additionally takes FROM and its joins") {
#if (SQLITE_VERSION_NUMBER >= 3033000)
        STATIC_REQUIRE(is_update_clause<From>::value);
        STATIC_REQUIRE(is_update_clause<Join>::value);
#else
        STATIC_REQUIRE_FALSE(is_update_clause<From>::value);
        STATIC_REQUIRE_FALSE(is_update_clause<Join>::value);
#endif
        STATIC_REQUIRE(is_update_clause<Where>::value);
        STATIC_REQUIRE(is_update_clause<OrderBy>::value);
        STATIC_REQUIRE(is_update_clause<Limit>::value);
        STATIC_REQUIRE_FALSE(is_update_clause<GroupBy>::value);
        STATIC_REQUIRE_FALSE(is_update_clause<Window>::value);
    }
    SECTION("each keeps its own canonical order") {
        STATIC_REQUIRE(check_delete_clause_order_v<std::tuple<Where, OrderBy, Limit>>);
        STATIC_REQUIRE_FALSE(check_delete_clause_order_v<std::tuple<Limit, Where>>);
        STATIC_REQUIRE(check_update_clause_order_v<std::tuple<Where, OrderBy, Limit>>);
        STATIC_REQUIRE_FALSE(check_update_clause_order_v<std::tuple<OrderBy, Where>>);
    }
    //  both lists are subsets of the select clauses, which is what `is_statement_clause` relies on
    SECTION("every DML clause is also a select clause") {
        STATIC_REQUIRE(is_select_clause<Where>::value);
        STATIC_REQUIRE(is_select_clause<OrderBy>::value);
        STATIC_REQUIRE(is_select_clause<Limit>::value);
        STATIC_REQUIRE(is_statement_clause<Where>::value);
    }
}
