#include <sqlite_orm/sqlite_orm.h>
#ifdef SQLITE_ORM_WITH_CTE
#include <type_traits>  //  std::is_same, std::is_constructible
#include <tuple>  //  std::ignore
#include <string>  //  std::string
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::alias_holder, internal::column_alias;
using internal::column_t;
using std::is_same, std::is_constructible;
using std::tuple;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
using internal::using_t;
#endif

template<class T, class E>
void do_assert() {
    STATIC_REQUIRE(is_same<T, E>::value);
}

template<class E, class ColRef>
void runDecay(ColRef /*colRef*/) {
    do_assert<internal::transform_tuple_t<tuple<ColRef>, internal::decay_explicit_column_t>, E>();
}

template<class E, class T>
void runTest(T /*test*/) {
    do_assert<T, E>();
}

TEST_CASE("CTE type traits") {
    SECTION("decay_explicit_column") {
        struct Org {
            int64 id = 0;
            int64 getId() const {
                return this->id;
            }
        };

        STATIC_REQUIRE(is_constructible<alias_holder<column_alias<'c'>>, column_alias<'c'>>::value);
        runDecay<tuple<decltype(&Org::id)>>(&Org::id);
        runDecay<tuple<decltype(&Org::getId)>>(&Org::getId);
        runDecay<tuple<alias_holder<column_alias<'1'>>>>(1_colalias);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        runDecay<tuple<alias_holder<column_alias<'a'>>>>("a"_col);
#endif
        runDecay<tuple<column_t<int64 Org::*, internal::empty_setter>>>(make_column("id", &Org::id));
        runDecay<tuple<polyfill::remove_cvref_t<decltype(std::ignore)>>>(std::ignore);
        runDecay<tuple<std::string>>("");
    }
}

TEST_CASE("CTE building") {
    SECTION("moniker") {
        constexpr auto cte1 = 1_ctealias;
        using cte_1 = decltype(1_ctealias);
        runTest<internal::cte_moniker<'1'>>(1_ctealias);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        runTest<internal::cte_moniker<'z'>>("z"_cte);
        STATIC_REQUIRE(internal::is_cte_moniker_v<cte_1>);
        STATIC_REQUIRE(orm_cte_moniker<cte_1>);
        STATIC_REQUIRE(internal::is_recordset_alias_v<cte_1>);
        STATIC_REQUIRE(orm_recordset_alias<cte_1>);
        STATIC_REQUIRE_FALSE(internal::is_table_alias_v<cte_1>);
        STATIC_REQUIRE_FALSE(orm_table_alias<cte_1>);
#endif
    }
    SECTION("builder") {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto cte1 = 1_ctealias;
        auto builder1 = cte<1_ctealias>();
#else
        using cte_1 = decltype(1_ctealias);
        auto builder1 = cte<cte_1>();
#endif
        auto builder2 = 1_ctealias();
        STATIC_REQUIRE(std::is_same<decltype(builder2), decltype(builder1)>::value);
    }
}

TEST_CASE("CTE storage") {
    SECTION("db_objects_cat") {
        struct Org {
            int64 id = 0;
        };

        auto table = make_table("org", make_column("id", &Org::id));
        auto idx1 = make_unique_index("idx1_org", &Org::id);
        auto idx2 = make_index("idx2_org", &Org::id);
        auto dbObjects = tuple{idx1, idx2, table};
        auto cteTable = internal::make_cte_table(dbObjects, 1_ctealias().as(select(1)));
        auto dbObjects2 = internal::db_objects_cat(dbObjects, cteTable);

        // note: deliberately make indexes resulting in the same index_t<> type, such that we know `db_objects_cat()` is working properly
        STATIC_REQUIRE(is_same<decltype(idx1), decltype(idx2)>::value);
        STATIC_REQUIRE(is_same<decltype(dbObjects2),
                               tuple<decltype(cteTable), decltype(idx1), decltype(idx2), decltype(table)>>::value);
    }
}

TEST_CASE("CTE expressions") {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto cte1 = 1_ctealias;
    using cte_1 = decltype(1_ctealias);
    constexpr auto x = 1_colalias;
    using x_t = decltype(1_colalias);
    SECTION("moniker expressions") {
        runTest<internal::from_t<cte_1>>(from<cte1>());
        runTest<internal::asterisk_t<cte_1>>(asterisk<cte1>());
        runTest<internal::count_asterisk_t<cte_1>>(count<cte1>());
        runTest<internal::left_join_t<cte_1, using_t<cte_1, alias_holder<x_t>>>>(left_join<cte1>(using_(cte1->*x)));
        runTest<internal::join_t<cte_1, using_t<cte_1, alias_holder<x_t>>>>(join<cte1>(using_(cte1->*x)));
        runTest<internal::left_outer_join_t<cte_1, using_t<cte_1, alias_holder<x_t>>>>(
            left_outer_join<cte1>(using_(cte1->*x)));
        runTest<internal::inner_join_t<cte_1, using_t<cte_1, alias_holder<x_t>>>>(inner_join<cte1>(using_(cte1->*x)));
    }
#endif
}
#endif
