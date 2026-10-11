#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <string>  //  std::string
#include <type_traits>  //  std::is_same
#include <vector>  //  std::vector

using namespace sqlite_orm;
using std::is_same;

/*
 *  `c()` only quotes an operand into the DSL. Whichever way the operand is then passed - to an overloaded
 *  operator or to a named factory function - the resulting node holds the operand itself, never the quoting
 *  wrapper, so a node built from `c(x)` is the very node built from `x`.
 */
TEST_CASE("quoted operands") {
    struct User {
        int id = 0;
        std::string name;
    };

    SECTION("operator!") {
        STATIC_REQUIRE(is_same<decltype(!c(&User::id)), internal::negated_condition_t<int User::*>>::value);
        STATIC_REQUIRE(is_same<decltype(!c(20)), internal::negated_condition_t<int>>::value);
    }
    SECTION("comparison") {
        STATIC_REQUIRE(is_same<decltype(eq(c(&User::id), 5)), decltype(eq(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(eq(&User::id, c(5))), decltype(eq(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(ne(c(&User::id), 5)), decltype(ne(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(lt(c(&User::id), 5)), decltype(lt(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(le(c(&User::id), c(5))), decltype(le(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(gt(c(&User::id), 5)), decltype(gt(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(ge(c(&User::id), 5)), decltype(ge(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(is_equal(c(&User::id), 5)), decltype(is_equal(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(is_not_equal(c(&User::id), 5)), decltype(is_not_equal(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(less_than(c(&User::id), 5)), decltype(less_than(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(less_or_equal(c(&User::id), 5)), decltype(less_or_equal(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(greater_than(c(&User::id), 5)), decltype(greater_than(&User::id, 5))>::value);
        STATIC_REQUIRE(
            is_same<decltype(greater_or_equal(c(&User::id), 5)), decltype(greater_or_equal(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(is(c(&User::id), 5)), decltype(is(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(is_not(c(&User::id), 5)), decltype(is_not(&User::id, 5))>::value);
#if SQLITE_VERSION_NUMBER >= 3039000
        STATIC_REQUIRE(
            is_same<decltype(is_distinct_from(c(&User::id), 5)), decltype(is_distinct_from(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(is_not_distinct_from(c(&User::id), 5)),
                               decltype(is_not_distinct_from(&User::id, 5))>::value);
#endif
    }
    SECTION("logical") {
        STATIC_REQUIRE(is_same<decltype(and_(c(&User::id), c(5))), decltype(and_(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(or_(c(&User::id), c(5))), decltype(or_(&User::id, 5))>::value);
    }
    //  `||` is the logical OR for conditions and the string concatenation otherwise:
    //  a quoted condition is told apart as the condition it quotes
    SECTION("logical or concatenation") {
        auto cond1 = eq(&User::id, 1);
        auto cond2 = eq(&User::id, 2);
        STATIC_REQUIRE(internal::is_conditional_operand<decltype(c(cond1))>::value);
        STATIC_REQUIRE_FALSE(internal::is_conditional_operand<decltype(c(&User::name))>::value);
        STATIC_REQUIRE(is_same<decltype(c(cond1) || c(cond2)), decltype(cond1 || cond2)>::value);
        STATIC_REQUIRE(is_same<decltype(c(cond1) || cond2), decltype(cond1 || cond2)>::value);
        STATIC_REQUIRE(is_same<decltype(cond1 || c(cond2)), decltype(cond1 || cond2)>::value);
        STATIC_REQUIRE(
            is_same<decltype(c(cond1) || cond2), internal::or_condition_t<decltype(cond1), decltype(cond2)>>::value);
        STATIC_REQUIRE(is_same<decltype(c(cond1) && c(cond2)), decltype(cond1 && cond2)>::value);
        auto match1 = match(&User::name, "a");
        auto match2 = match(&User::name, "b");
        STATIC_REQUIRE(is_same<decltype(c(match1) || c(match2)),
                               internal::or_condition_t<decltype(match1), decltype(match2)>>::value);
        STATIC_REQUIRE(
            is_same<decltype(c(&User::name) || "a"), internal::conc_t<std::string User::*, const char*>>::value);
        STATIC_REQUIRE(
            is_same<decltype(c(&User::name) || c("a")), internal::conc_t<std::string User::*, const char*>>::value);
    }
    SECTION("arithmetic and bitwise") {
        STATIC_REQUIRE(is_same<decltype(add(c(&User::id), 5)), decltype(add(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(sub(c(&User::id), c(5))), decltype(sub(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(mul(c(&User::id), 5)), decltype(mul(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(div(c(&User::id), 5)), decltype(div(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(mod(c(&User::id), 5)), decltype(mod(&User::id, 5))>::value);
        STATIC_REQUIRE(is_same<decltype(conc(c(&User::name), "@")), decltype(conc(&User::name, "@"))>::value);
        STATIC_REQUIRE(
            is_same<decltype(bitwise_shift_left(c(&User::id), 1)), decltype(bitwise_shift_left(&User::id, 1))>::value);
        STATIC_REQUIRE(is_same<decltype(bitwise_shift_right(c(&User::id), 1)),
                               decltype(bitwise_shift_right(&User::id, 1))>::value);
        STATIC_REQUIRE(is_same<decltype(bitwise_and(c(&User::id), 1)), decltype(bitwise_and(&User::id, 1))>::value);
        STATIC_REQUIRE(is_same<decltype(bitwise_or(c(&User::id), 1)), decltype(bitwise_or(&User::id, 1))>::value);
        STATIC_REQUIRE(is_same<decltype(minus(c(&User::id))), decltype(minus(&User::id))>::value);
        STATIC_REQUIRE(is_same<decltype(bitwise_not(c(&User::id))), decltype(bitwise_not(&User::id))>::value);
        STATIC_REQUIRE(is_same<decltype(assign(c(&User::id), c(5))), decltype(assign(&User::id, 5))>::value);
    }
    SECTION("null tests") {
        STATIC_REQUIRE(is_same<decltype(is_null(c(&User::id))), decltype(is_null(&User::id))>::value);
        STATIC_REQUIRE(is_same<decltype(is_not_null(c(&User::id))), decltype(is_not_null(&User::id))>::value);
    }
    SECTION("pattern matching") {
        STATIC_REQUIRE(is_same<decltype(like(c(&User::name), "a%")), decltype(like(&User::name, "a%"))>::value);
        STATIC_REQUIRE(
            is_same<decltype(like(c(&User::name), "a%", "!")), decltype(like(&User::name, "a%", "!"))>::value);
        STATIC_REQUIRE(is_same<decltype(glob(c(&User::name), "a*")), decltype(glob(&User::name, "a*"))>::value);
        STATIC_REQUIRE(is_same<decltype(match(c(&User::name), "a")), decltype(match(&User::name, "a"))>::value);
        STATIC_REQUIRE(is_same<decltype(match(&User::name, c("a"))), decltype(match(&User::name, "a"))>::value);
        STATIC_REQUIRE(is_same<decltype(match(c(column<User>(&User::name)), c("a"))),
                               decltype(match(column<User>(&User::name), "a"))>::value);
    }
    SECTION("ranges and sets") {
        STATIC_REQUIRE(is_same<decltype(between(c(&User::id), 1, 10)), decltype(between(&User::id, 1, 10))>::value);
        STATIC_REQUIRE(is_same<decltype(in(c(&User::id), {1, 2})), decltype(in(&User::id, {1, 2}))>::value);
        STATIC_REQUIRE(is_same<decltype(in(c(&User::id), std::vector<int>{1, 2})),
                               decltype(in(&User::id, std::vector<int>{1, 2}))>::value);
        STATIC_REQUIRE(is_same<decltype(not_in(c(&User::id), {1, 2})), decltype(not_in(&User::id, {1, 2}))>::value);
    }
}
