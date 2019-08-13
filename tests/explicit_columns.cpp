#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Explicit colums"){
    struct Object {
        int id;
    };
    
    struct User : Object {
        std::string name;
        
        User(decltype(id) id_, decltype(name) name_): Object{id_}, name(std::move(name_)) {}
    };
    
    struct Token : Object {
        std::string token;
        int usedId;
        
        Token(decltype(id) id_, decltype(token) token_, decltype(usedId) usedId_): Object{id_}, token(std::move(token_)), usedId(usedId_) {}
    };
    
    auto storage = make_storage("column_pointer.sqlite",
                                make_table<User>("users",
                                                 make_column("id", &User::id, primary_key()),
                                                 make_column("name", &User::name)),
                                make_table<Token>("tokens",
                                                  make_column("id", &Token::id, primary_key()),
                                                  make_column("token", &Token::token),
                                                  make_column("used_id", &Token::usedId),
                                                  foreign_key(&Token::usedId).references(column<User>(&User::id))));
    storage.sync_schema();
    REQUIRE(storage.table_exists("users"));
    REQUIRE(storage.table_exists("tokens"));
    
    storage.remove_all<Token>();
    storage.remove_all<User>();
    
    auto brunoId = storage.insert(User{0, "Bruno"});
    auto zeddId = storage.insert(User{0, "Zedd"});
    
    REQUIRE(storage.count<User>() == 2);
    
    {
        auto w = where(is_equal(&User::name, "Bruno"));
        
        {
            auto rows = storage.select(column<User>(&User::id), w);
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.front() == brunoId);
        }
        
        {
            auto rows2 = storage.select(columns(column<User>(&User::id)), w);
            REQUIRE(rows2.size() == 1);
            REQUIRE(std::get<0>(rows2.front()) == brunoId);
            
            auto rows3 = storage.select(columns(column<User>(&Object::id)), w);
            REQUIRE(rows3 == rows2);
        }
    }
    
    {
        auto rows = storage.select(column<User>(&User::id), where(is_equal(&User::name, "Zedd")));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == zeddId);
    }
    
    {
        auto abcId = storage.insert(Token(0, "abc", brunoId));
        
        auto w = where(is_equal(&Token::token, "abc"));
        {
            auto rows = storage.select(column<Token>(&Token::id), w);
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.front() == abcId);
        }
        
        {
            auto rows2 = storage.select(columns(column<Token>(&Token::id), &Token::usedId), w);
            REQUIRE(rows2.size() == 1);
            REQUIRE(std::get<0>(rows2.front()) == abcId);
            REQUIRE(std::get<1>(rows2.front()) == brunoId);
        }
    }
    
    {
        auto joinedRows = storage.select(columns(&User::name, &Token::token),
                                         join<Token>(on(is_equal(&Token::usedId, column<User>(&User::id)))));
        REQUIRE(joinedRows.size() == 1);
        REQUIRE(std::get<0>(joinedRows.front()) == "Bruno");
        REQUIRE(std::get<1>(joinedRows.front()) == "abc");
    }
}
