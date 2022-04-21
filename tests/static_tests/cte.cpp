#include <type_traits>  //  std::is_same, std::is_constructible
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
using internal::alias_holder;
using internal::column_alias;
using internal::column_t;
using std::tuple;

template<class ColRefs, class E>
void do_assert() {
    STATIC_REQUIRE(std::is_same<ColRefs, E>::value);
}

template<class E, class ColRef>
void runTest(ColRef colRef) {
    do_assert<internal::transform_tuple_t<tuple<ColRef>, internal::decay_explicit_column>, E>();
}

TEST_CASE("CTE type traits") {
    SECTION("decay_explicit_column") {
        struct Org {
            int64 id = 0;
            int64 getId() const {
                return this->id;
            }
        };

        STATIC_REQUIRE(std::is_constructible<alias_holder<column_alias<'c'>>, column_alias<'c'>>::value);
        runTest<tuple<decltype(&Org::id)>>(&Org::id);
        runTest<tuple<decltype(&Org::getId)>>(&Org::getId);
        runTest<tuple<alias_holder<column_alias<'1'>>>>(1_colalias);
#if __cpp_nontype_template_args >= 201911
        runTest<tuple<alias_holder<column_alias<'a'>>>>("a"_col);
#endif
        runTest<tuple<column_t<Org, int64, int64 Org::*, internal::empty_setter>>>(make_column("id", &Org::id));
        runTest<tuple<polyfill::remove_cvref_t<decltype(std::ignore)>>>(std::ignore);
        runTest<tuple<std::string>>("");
    }
}
