#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Like operator"){
    struct User {
        int id = 0;
        std::string name;
    };
    struct Pattern {
        std::string value;
    };
    
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id, autoincrement(), primary_key()),
                                           make_column("name", &User::name)),
                                make_table("patterns",
                                           make_column("value", &Pattern::value)));
    storage.sync_schema();
    
    storage.insert(User{0, "Sia"});
    storage.insert(User{0, "Stark"});
    storage.insert(User{0, "Index"});
    
    auto whereCondition = where(like(&User::name, "S%"));
    {
        auto users = storage.get_all<User>(whereCondition);
        REQUIRE(users.size() == 2);
    }
    {
        auto rows = storage.select(&User::id, whereCondition);
        REQUIRE(rows.size() == 2);
    }
    {
        auto rows = storage.select(like("ototo", "ot_to"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == true);
    }
    {
        auto rows = storage.select(like(&User::name, "S%a"));
        REQUIRE(rows.size() == 3);
        REQUIRE(count_if(rows.begin(), rows.end(), [](bool arg){
            return arg == true;
        }) == 1);
    }
    
    storage.insert(Pattern{"o%o"});
    {
        auto rows = storage.select(like("ototo", &Pattern::value));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == true);
    }
    {
        auto rows = storage.select(like("aaa", &Pattern::value));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == false);
    }
    {
        auto rows = storage.select(like("ototo", "ototo%").escape("%"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == true);
    }
    {
        auto rows = storage.select(like("ototo", "ototo%", "%"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == true);
    }
}
