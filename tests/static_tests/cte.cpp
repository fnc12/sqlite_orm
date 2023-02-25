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

template<class ColRefs, class E>
void do_assert() {
    STATIC_REQUIRE(is_same<ColRefs, E>::value);
}

template<class E, class ColRef>
void runTest(ColRef /*colRef*/) {
    do_assert<internal::transform_tuple_t<tuple<ColRef>, internal::decay_explicit_column_t>, E>();
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
        runTest<tuple<decltype(&Org::id)>>(&Org::id);
        runTest<tuple<decltype(&Org::getId)>>(&Org::getId);
        runTest<tuple<alias_holder<column_alias<'1'>>>>(1_colalias);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        runTest<tuple<alias_holder<column_alias<'a'>>>>("a"_col);
#endif
        runTest<tuple<column_t<int64 Org::*, internal::empty_setter>>>(make_column("id", &Org::id));
        runTest<tuple<polyfill::remove_cvref_t<decltype(std::ignore)>>>(std::ignore);
        runTest<tuple<std::string>>("");
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
        auto cteTable = internal::make_cte_table(dbObjects, cte<cte_1>()(select(1)));
        auto dbObjects2 = internal::db_objects_cat(dbObjects, cteTable);

        // note: deliberately make indexes resulting in the same index_t<> type, such that we know `db_objects_cat()` is working properly
        static_assert(is_same<decltype(idx1), decltype(idx2)>::value);
        static_assert(is_same<decltype(dbObjects2),
                              tuple<decltype(cteTable), decltype(idx1), decltype(idx2), decltype(table)>>::value);
    }
}
#endif
