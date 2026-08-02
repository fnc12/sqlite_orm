#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_WITH_VIEW
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
using namespace sqlite_orm;
using internal::col_index_sequence_of, internal::col_index_sequence_with_field_type;
using internal::is_column;

namespace {
    struct[[= "user_view"_orm_name]] UserViewStaticTests {
        int64 id = 0;
        std::string name;

      private:
        std::string _privateDummy;
    };
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_reference auto user_view = c<UserViewStaticTests>();
#endif
}

TEST_CASE("view static") {
    STATIC_REQUIRE(std::is_same_v<decltype(orm_name("user_view")), decltype("user_view"_orm_name)>);
}

TEST_CASE("view static count_of<is_column>()") {
    struct User {
        int64 id = 0;
        std::string name;
    };

    SECTION("traditional") {
        auto view = make_view<UserViewStaticTests>(select(asterisk<User>()));
        using elements_type = decltype(view.elements);
        STATIC_REQUIRE(view.count_of<is_column>() == 2);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
    }
    SECTION("derived") {
        struct DerivedUserView : UserViewStaticTests {};

        auto view = make_view<DerivedUserView>(select(asterisk<User>()));
        using elements_type = decltype(view.elements);
        STATIC_REQUIRE(view.count_of<is_column>() == 2);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("table reference") {
        auto view = make_view<user_view>(select(asterisk<User>()));
        using elements_type = decltype(view.elements);
        STATIC_REQUIRE(view.count_of<is_column>() == 2);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
    }
#endif
}
#endif
#endif
