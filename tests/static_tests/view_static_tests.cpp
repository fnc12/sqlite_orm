#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_WITH_VIEW
using namespace sqlite_orm;
using internal::is_column;

struct UserViewStaticTests {
    int id = 0;
    std::string name;
};

TEST_CASE("view static count_of<is_column>()") {
    struct User {
        int id = 0;
        std::string name;
    };

    {
        auto view = make_view<UserViewStaticTests>("user_view", select(columns(&User::id, &User::name)));
        STATIC_REQUIRE(view.count_of<is_column>() == 2);
    }
}
#endif
