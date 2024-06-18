#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if defined(SQLITE_ORM_WITH_CPP20_ALIASES) || defined(SQLITE_ORM_WITH_CTE)
using namespace sqlite_orm::literals;
#endif
using sqlite_orm::alias_a;
using sqlite_orm::alias_column;
using sqlite_orm::and_;
using sqlite_orm::c;
using sqlite_orm::colalias_a;
using sqlite_orm::column;
using sqlite_orm::func;
using sqlite_orm::get;
using sqlite_orm::or_;
using sqlite_orm::internal::and_condition_t;
using sqlite_orm::internal::binary_operator;
using sqlite_orm::internal::greater_or_equal_t;
using sqlite_orm::internal::greater_than_t;
using sqlite_orm::internal::is_equal_t;
using sqlite_orm::internal::is_not_equal_t;
using sqlite_orm::internal::less_or_equal_t;
using sqlite_orm::internal::less_than_t;
using sqlite_orm::internal::negated_condition_t;
using sqlite_orm::internal::or_condition_t;
using sqlite_orm::polyfill::is_specialization_of_v;

template<class E>
void runTests(E expression) {
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression < 42), less_than_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 < expression), less_than_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression < expression), less_than_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression <= 42), less_or_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 <= expression), less_or_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression <= expression), less_or_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression > 42), greater_than_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 > expression), greater_than_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression > expression), greater_than_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression >= 42), greater_or_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 >= expression), greater_or_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression >= expression), greater_or_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression == 42), is_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 == expression), is_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression == expression), is_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression == 42 == 1), is_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(1 == (42 == expression)), is_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression != 42), is_not_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 != expression), is_not_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression != expression), is_not_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression != 42 != 0), is_not_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(0 != (42 != expression)), is_not_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression || 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 || expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression || expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression || 42 || 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression || 42 || c(42)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 || (expression || 42)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(42) || (expression || 42)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression + 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 + expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression + expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 + expression + 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 + (expression + 42)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression - 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 - expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression - expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 - expression - 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 - (expression - 42)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression * 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 * expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression * expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 * expression * 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 * (expression * 42)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression / 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 / expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression / expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 / expression / 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 / (expression / 42)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(expression % 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 % expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression % expression), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 % expression % 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 % (expression % 42)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(!expression), negated_condition_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(!expression || expression), or_condition_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(!expression || 42), or_condition_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 || !expression), or_condition_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression && 43), and_condition_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(expression && expression), and_condition_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(!expression && 42), and_condition_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 && !expression), and_condition_t>);

    // conc_t + condition_t yield or_condition_t
    STATIC_REQUIRE(is_specialization_of_v<decltype((expression && 42) || !expression), or_condition_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(!expression || (expression && 42)), or_condition_t>);
}

TEST_CASE("inline namespace literals expressions") {
#ifdef SQLITE_ORM_WITH_CTE
    constexpr auto col1 = 1_colalias;
    constexpr auto cte1 = 1_ctealias;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto cte_mnkr = "1"_cte;
#endif
#endif
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto u_alias_builder = "u"_alias;
    constexpr auto c_col = "c"_col;
    constexpr auto f_scalar_builder = "f"_scalar;
#if SQLITE_VERSION_NUMBER >= 3020000
    constexpr auto domain_ptr_tag = "domain"_pointer_type;
#endif
#endif
}

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
TEST_CASE("ADL and pointer-to-member expressions") {
    struct User {
        int id;
    };
    constexpr auto user_table = c<User>();
    constexpr auto u_alias = "u"_alias.for_<User>();
    constexpr auto cte = "1"_cte;

    user_table->*&User::id;
    u_alias->*&User::id;
    cte->*&User::id;
}
#endif

TEST_CASE("ADL and expression operators") {
    struct User {
        int id;
    };
    struct ScalarFunction {
        static const char* name();
        int operator()() const;
    };

    runTests(c(&User::id));
    runTests(column<User>(&User::id));
    runTests(get<colalias_a>());
    runTests(alias_column<alias_a<User>>(&User::id));
    runTests(func<ScalarFunction>());
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    runTests("a"_col);
#endif
}
