//
//  custom_functions.cpp
//  CPPTest
//
//  Created by John Zakharov on 07.04.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#include <sqlite_orm/sqlite_orm.h>

using namespace sqlite_orm;

struct MyFunc : custom_function<MyFunc, int(std::string)> {
    
    static int run(std::string arg) {
        return (int)arg.length();
    }
    
    operator std::string() const {
        return "MyLen";
    }
};

struct User {
    int id;
    std::string name;
};

void xFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
    ff
}

void xStep(sqlite3_context *, int, sqlite3_value **){
    ff
}

void xFinal(sqlite3_context *){
    ff
}

int main() {
    
    auto storage = make_storage("custom_funcitons.sqlite",
                                make_table("users",
                                           make_column("id",
                                                       &User::id,
                                                       primary_key()),
                                           make_column("name",
                                                       &User::name)));
    storage.on_open = [](sqlite3 *db){
        sqlite3_create_function(db, "ototo", 1, SQLITE_UTF8, nullptr, xFunc, xStep, xFinal);
    };
    storage.sync_schema();
    auto rows = storage.select(func<MyFunc>(&User::name));
    
    
    
    
    return 0;
}
