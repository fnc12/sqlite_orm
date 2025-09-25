#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_WITH_VIEW
using namespace sqlite_orm;
using internal::col_index_sequence_of, internal::col_index_sequence_with_field_type;
using internal::is_column;

struct UserViewStaticTests {
    int id = 0;
    std::string name;
};
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
constexpr orm_table_reference auto user_view = c<UserViewStaticTests>();
#endif

TEST_CASE("view static count_of<is_column>()") {
    struct User {
        int id = 0;
        std::string name;
    };

    SECTION("traditional") {
        auto view = make_view<UserViewStaticTests>("user_view", select(columns(&User::id, &User::name)));
        using elements_type = decltype(view.elements);
        STATIC_REQUIRE(view.count_of<is_column>() == 2);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("table reference") {
        auto view = make_view<user_view>("user_view", select(columns(&User::id, &User::name)));
        using elements_type = decltype(view.elements);
        STATIC_REQUIRE(view.count_of<is_column>() == 2);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
    }
#endif
}
#endif
