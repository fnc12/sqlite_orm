#include <sqlite_orm/sqlite_orm.h>

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <cassert>  //  assert
#include <vector>   //  std::vector
#include <string>   //  std::string
#include <iostream> //  std::cout, std::endl
#include <memory>   //  std::unique_ptr
#include <cstdio>   //  remove
#include <numeric>  //  std::iota
#include <algorithm>    //  std::fill

using namespace sqlite_orm;

using std::cout;
using std::endl;

TEST_CASE("Join iterator ctor compilation error") {
    //  TODO: move to static tests
    struct Tag {
        int objectId;
        std::string text;
    };
    
    auto storage = make_storage("join_error.sqlite",
                                make_table("tags",
                                           make_column("object_id", &Tag::objectId),
                                           make_column("text", &Tag::text)));
    storage.sync_schema();
    
    auto offs = 0;
    auto lim = 5;
    storage.select(columns(&Tag::text, count(&Tag::text)),
                   group_by(&Tag::text),
                   order_by(count(&Tag::text)).desc(),
                   limit(offs, lim));
}

TEST_CASE("limits"){
    auto storage2 = make_storage("limits.sqlite");
    auto storage = storage2;
    storage.sync_schema();

    {
        auto length = storage.limit.length();
        auto newLength = length - 10;
        storage.limit.length(newLength);
        length = storage.limit.length();
        REQUIRE(length == newLength);
    }
    {
        auto sqlLength = storage.limit.sql_length();
        auto newSqlLength = sqlLength - 10;
        storage.limit.sql_length(newSqlLength);
        sqlLength = storage.limit.sql_length();
        REQUIRE(sqlLength == newSqlLength);
    }
    {
        auto column = storage.limit.column();
        auto newColumn = column - 10;
        storage.limit.column(newColumn);
        column = storage.limit.column();
        REQUIRE(column == newColumn);
    }
    {
        auto exprDepth = storage.limit.expr_depth();
        auto newExprDepth = exprDepth - 10;
        storage.limit.expr_depth(newExprDepth);
        exprDepth = storage.limit.expr_depth();
        REQUIRE(exprDepth == newExprDepth);
    }
    {
        auto compoundSelect = storage.limit.compound_select();
        auto newCompoundSelect = compoundSelect - 10;
        storage.limit.compound_select(newCompoundSelect);
        compoundSelect = storage.limit.compound_select();
        REQUIRE(compoundSelect == newCompoundSelect);
    }
    {
        auto vdbeOp = storage.limit.vdbe_op();
        auto newVdbe_op = vdbeOp - 10;
        storage.limit.vdbe_op(newVdbe_op);
        vdbeOp = storage.limit.vdbe_op();
        REQUIRE(vdbeOp == newVdbe_op);
    }
    {
        auto functionArg = storage.limit.function_arg();
        auto newFunctionArg = functionArg - 10;
        storage.limit.function_arg(newFunctionArg);
        functionArg = storage.limit.function_arg();
        REQUIRE(functionArg == newFunctionArg);
    }
    {
        auto attached = storage.limit.attached();
        auto newAttached = attached - 1;
        storage.limit.attached(newAttached);
        attached = storage.limit.attached();
        REQUIRE(attached == newAttached);
    }
    {
        auto likePatternLength = storage.limit.like_pattern_length();
        auto newLikePatternLength = likePatternLength - 10;
        storage.limit.like_pattern_length(newLikePatternLength);
        likePatternLength = storage.limit.like_pattern_length();
        REQUIRE(likePatternLength == newLikePatternLength);
    }
    {
        auto variableNumber = storage.limit.variable_number();
        auto newVariableNumber = variableNumber - 10;
        storage.limit.variable_number(newVariableNumber);
        variableNumber = storage.limit.variable_number();
        REQUIRE(variableNumber == newVariableNumber);
    }
    {
        auto triggerDepth = storage.limit.trigger_depth();
        auto newTriggerDepth = triggerDepth - 10;
        storage.limit.trigger_depth(newTriggerDepth);
        triggerDepth = storage.limit.trigger_depth();
        REQUIRE(triggerDepth == newTriggerDepth);
    }
    {
        auto workerThreads = storage.limit.worker_threads();
        auto newWorkerThreads = workerThreads + 1;
        storage.limit.worker_threads(newWorkerThreads);
        workerThreads = storage.limit.worker_threads();
        REQUIRE(workerThreads == newWorkerThreads);
    }
}

TEST_CASE("Explicit insert"){
    struct User {
        int id;
        std::string name;
        int age;
        std::string email;
    };
    
    class Visit {
    public:
        const int& id() const {
            return _id;
        }
        
        void setId(int newValue) {
            _id = newValue;
        }
        
        const time_t& createdAt() const {
            return _createdAt;
        }
        
        void setCreatedAt(time_t newValue) {
            _createdAt = newValue;
        }
        
        const int& usedId() const {
            return _usedId;
        }
        
        void setUsedId(int newValue) {
            _usedId = newValue;
        }
        
    private:
        int _id;
        time_t _createdAt;
        int _usedId;
    };
    
    auto storage = make_storage("explicitinsert.sqlite",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name),
                                           make_column("age", &User::age),
                                           make_column("email", &User::email, default_value("dummy@email.com"))),
                                make_table("visits",
                                           make_column("id", &Visit::setId, &Visit::id, primary_key()),
                                           make_column("created_at", &Visit::createdAt, &Visit::setCreatedAt, default_value(10)),
                                           make_column("used_id", &Visit::usedId, &Visit::setUsedId)));
    
    storage.sync_schema();
    storage.remove_all<User>();
    storage.remove_all<Visit>();
    
    {
        
        {
            User user{};
            user.name = "Juan";
            user.age = 57;
            auto id = storage.insert(user, columns(&User::name, &User::age));
            REQUIRE(storage.get<User>(id).email == "dummy@email.com");
        }
        
        {
            User user2;
            user2.id = 2;
            user2.name = "Kevin";
            user2.age = 27;
            REQUIRE(user2.id == storage.insert(user2, columns(&User::id, &User::name, &User::age)));
            REQUIRE(storage.get<User>(user2.id).email == "dummy@email.com");
        }
        
        {
            User user3;
            user3.id = 3;
            user3.name = "Sia";
            user3.age = 42;
            user3.email = "sia@gmail.com";
            auto insertedId = storage.insert(user3, columns(&User::id, &User::name, &User::age, &User::email));
            REQUIRE(user3.id == insertedId);
            auto insertedUser3 = storage.get<User>(user3.id);
            REQUIRE(insertedUser3.email == user3.email);
            REQUIRE(insertedUser3.age == user3.age);
            REQUIRE(insertedUser3.name == user3.name);
        }
        
        {
            User user4;
            user4.name = "Egor";
            try {
                storage.insert(user4, columns(&User::name));
                REQUIRE(false);
//                throw std::runtime_error("Must not fire");
            } catch (const std::system_error&) {
                //        cout << e.what() << endl;
            }
        }
    }
    {
        
        {
            Visit visit;
            
            {
                visit.setUsedId(1);
                visit.setId(storage.insert(visit, columns(&Visit::usedId)));
                
                auto visitFromStorage = storage.get<Visit>(visit.id());
                REQUIRE(visitFromStorage.createdAt() == 10);
                REQUIRE(visitFromStorage.usedId() == visit.usedId());
                storage.remove<Visit>(visitFromStorage.usedId());
            }
            
            {
                visit.setId(storage.insert(visit, columns(&Visit::setUsedId)));
                auto visitFromStorage = storage.get<Visit>(visit.id());
                REQUIRE(visitFromStorage.createdAt() == 10);
                REQUIRE(visitFromStorage.usedId() == visit.usedId());
                storage.remove<Visit>(visitFromStorage.usedId());
            }
            
            {
                Visit visit2;
                visit2.setId(2);
                visit2.setUsedId(1);
                {
                    auto insertedId = storage.insert(visit2, columns(&Visit::id, &Visit::usedId));
                    REQUIRE(visit2.id() == insertedId);
                    auto visitFromStorage = storage.get<Visit>(visit2.id());
                    REQUIRE(visitFromStorage.usedId() == visit2.usedId());
                    storage.remove<Visit>(visit2.id());
                }
                {
                    auto insertedId = storage.insert(visit2, columns(&Visit::setId, &Visit::setUsedId));
                    REQUIRE(visit2.id() == insertedId);
                    auto visitFromStorage = storage.get<Visit>(visit2.id());
                    REQUIRE(visitFromStorage.usedId() == visit2.usedId());
                    storage.remove<Visit>(visit2.id());
                }
            }
            
            {
                Visit visit3;
                visit3.setId(10);
                try {
                    storage.insert(visit3, columns(&Visit::id));
                    REQUIRE(false);
                } catch (const std::system_error&) {
                    //        cout << e.what() << endl;
                }
                
                try {
                    storage.insert(visit3, columns(&Visit::setId));
                    REQUIRE(false);
                } catch (const std::system_error&) {
                    //        cout << e.what() << endl;
                }
            }
        }
    }
}

TEST_CASE("Custom collate"){
    struct Item {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("custom_collate.sqlite",
                                make_table("items",
                                           make_column("id", &Item::id, primary_key()),
                                           make_column("name", &Item::name)));
//    storage.open_forever();
    storage.sync_schema();
    storage.remove_all<Item>();
    storage.insert(Item{ 0, "Mercury" });
    storage.insert(Item{ 0, "Mars" });
    storage.create_collation("ototo", [](int, const void *lhs, int, const void *rhs){
        return strcmp((const char*)lhs, (const char*)rhs);
    });
    storage.create_collation("alwaysequal", [](int, const void *, int, const void *){
        return 0;
    });
    auto rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo")));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "Mercury");
    
    rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                          order_by(&Item::name).collate("ototo"));
    
    storage.create_collation("ototo", {});
    try {
        rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo")));
    } catch (const std::system_error& e) {
        cout << e.what() << endl;
    }
    try {
        rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo2")));
    } catch (const std::system_error& e) {
        cout << e.what() << endl;
    }
    rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                                             order_by(&Item::name).collate_rtrim());
    
    rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                          order_by(&Item::name).collate("alwaysequal"));
    REQUIRE(rows.size() == static_cast<size_t>(storage.count<Item>()));
}

TEST_CASE("Vacuum"){
    struct Item {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("vacuum.sqlite",
                                make_table("items",
                                           make_column("id", &Item::id, primary_key()),
                                           make_column("name", &Item::name)));
    storage.sync_schema();
    storage.insert(Item{ 0, "One" });
    storage.insert(Item{ 0, "Two" });
    storage.insert(Item{ 0, "Three" });
    storage.insert(Item{ 0, "Four" });
    storage.insert(Item{ 0, "Five" });
    storage.remove_all<Item>();
    storage.vacuum();
}

TEST_CASE("Remove all"){
    struct Object {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("",
                                make_table("objects",
                                           make_column("id", &Object::id, primary_key()),
                                           make_column("name", &Object::name)));
    storage.sync_schema();
    
    storage.replace(Object{ 1, "Ototo" });
    storage.replace(Object{ 2, "Contigo" });
    
    REQUIRE(storage.count<Object>() == 2);
    
    storage.remove_all<Object>(where(c(&Object::id) == 1));
    
    REQUIRE(storage.count<Object>() == 1);
}

TEST_CASE("Escaped index name"){
    struct User{
        std::string group;
    };
    auto storage = make_storage("index_group.sqlite",
                                make_index("index", &User::group),
                                make_table("users",
                                           make_column("group", &User::group)));
    storage.sync_schema();
}

TEST_CASE("Where"){
    struct User{
        int id = 0;
        std::string name;
    };
    
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.replace(User{ 1, "Jeremy" });
    storage.replace(User{ 2, "Nataly" });
    
    auto users = storage.get_all<User>();
    REQUIRE(users.size() == 2);
    
    auto users2 = storage.get_all<User>(where(true));
    REQUIRE(users2.size() == 2);
    
    auto users3 = storage.get_all<User>(where(false));
    REQUIRE(users3.size() == 0);
    
    auto users4 = storage.get_all<User>(where(true and c(&User::id) == 1));
    REQUIRE(users4.size() == 1);
    REQUIRE(users4.front().id == 1);
    
    auto users5 = storage.get_all<User>(where(false and c(&User::id) == 1));
    REQUIRE(users5.size() == 0);
}
