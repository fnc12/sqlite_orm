#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Dynamic order by") {
    struct User {
        int id = 0;
        std::string firstName;
        std::string lastName;
        long registerTime = 0;
    };
    
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("first_name", &User::firstName),
                                           make_column("last_name", &User::lastName),
                                           make_column("register_time", &User::registerTime)));
    storage.sync_schema();
    
    storage.replace(User{1, "Jack", "Johnson", 100});
    storage.replace(User{2, "John", "Jackson", 90});
    storage.replace(User{3, "Elena", "Alexandra", 80});
    storage.replace(User{4, "Kaye", "Styles", 70});
    
    auto orderBy = dynamic_order_by(storage);
    std::vector<decltype(User::id)> expectedIds;
    
    SECTION("id") {
        orderBy.push_back(order_by(&User::id));
        expectedIds = {
            1, 2, 3, 4,
        };
    }
    
    SECTION("id desc") {
        orderBy.push_back(order_by(&User::id).desc());
        expectedIds = {
            4, 3, 2, 1,
        };
    }
    
    SECTION("firstName") {
        orderBy.push_back(order_by(&User::firstName));
        expectedIds = {
            3, 1, 2, 4,
        };
    }
    
    SECTION("firstName asc") {
        orderBy.push_back(order_by(&User::firstName).asc());
        expectedIds = {
            3, 1, 2, 4,
        };
    }
    
    SECTION("firstName desc") {
        orderBy.push_back(order_by(&User::firstName).desc());
        expectedIds = {
            4, 2, 1, 3,
        };
    }
    
    SECTION("firstName asc + id desc") {
        orderBy.push_back(order_by(&User::firstName).asc());
        orderBy.push_back(order_by(&User::id).desc());
        expectedIds = {
            3, 1, 2, 4,
        };
    }
    
    SECTION("lastName + firstName + id") {
        orderBy.push_back(order_by(&User::lastName));
        orderBy.push_back(order_by(&User::firstName));
        orderBy.push_back(order_by(&User::id));
        expectedIds = {
            3, 2, 1, 4,
        };
    }
    
    SECTION("lastName + firstName desc + id") {
        orderBy.push_back(order_by(&User::lastName));
        orderBy.push_back(order_by(&User::firstName).desc());
        orderBy.push_back(order_by(&User::id));
        expectedIds = {
            3, 2, 1, 4,
        };
    }
    
    auto rows = storage.get_all<User>(orderBy);
    REQUIRE(rows.size() == 4);
    for(auto i = 0; i < int(rows.size()); ++i) {
        auto &row = rows[i];
        REQUIRE(row.id == expectedIds[i]);
    }
    orderBy.clear();
}
