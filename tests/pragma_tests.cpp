#include <sqlite_orm/sqlite_orm.h>
#include <cstdio>   //  ::remove
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Journal mode"){
    auto filename = "journal_mode.sqlite";
    ::remove(filename);
    auto storage = make_storage(filename);
    auto jm = storage.pragma.journal_mode();
    REQUIRE(jm == decltype(jm)::DELETE);
    
    for(auto i = 0; i < 2; ++i){
        if(i == 0) {
            storage.begin_transaction();
        }
        storage.pragma.journal_mode(journal_mode::MEMORY);
        jm = storage.pragma.journal_mode();
        REQUIRE(jm == decltype(jm)::MEMORY);
        
        if(i == 1) {    //  WAL cannot be set within a transaction
            storage.pragma.journal_mode(journal_mode::WAL);
            jm = storage.pragma.journal_mode();
            REQUIRE(jm == decltype(jm)::WAL);
        }
        
        storage.pragma.journal_mode(journal_mode::OFF);
        jm = storage.pragma.journal_mode();
        REQUIRE(jm == decltype(jm)::OFF);
        
        storage.pragma.journal_mode(journal_mode::PERSIST);
        jm = storage.pragma.journal_mode();
        REQUIRE(jm == decltype(jm)::PERSIST);
        
        storage.pragma.journal_mode(journal_mode::TRUNCATE);
        jm = storage.pragma.journal_mode();
        REQUIRE(jm == decltype(jm)::TRUNCATE);
        
        if(i == 0) {
            storage.rollback();
        }
    }
}

TEST_CASE("Synchronous"){
    auto storage = make_storage("");
    const auto value = 1;
    storage.pragma.synchronous(value);
    REQUIRE(storage.pragma.synchronous() == value);
    
    storage.begin_transaction();
    
    const auto newValue = 2;
    try{
        storage.pragma.synchronous(newValue);
        throw std::runtime_error("Must not fire");
    }catch(const std::system_error&) {
        //  Safety level may not be changed inside a transaction
        REQUIRE(storage.pragma.synchronous() == value);
    }
    
    storage.commit();
}

TEST_CASE("User version"){
    auto storage = make_storage("");
    auto version = storage.pragma.user_version();
    
    storage.pragma.user_version(version + 1);
    REQUIRE(storage.pragma.user_version() == version + 1);
    
    storage.begin_transaction();
    storage.pragma.user_version(version + 2);
    REQUIRE(storage.pragma.user_version() == version + 2);
    storage.commit();
}

TEST_CASE("Auto vacuum"){
    auto filename = "autovacuum.sqlite";
    ::remove(filename);
    
    auto storage = make_storage(filename);
    
    storage.pragma.auto_vacuum(0);
    REQUIRE(storage.pragma.auto_vacuum() == 0);
    
    storage.pragma.auto_vacuum(1);
    REQUIRE(storage.pragma.auto_vacuum() == 1);
    
    storage.pragma.auto_vacuum(2);
    REQUIRE(storage.pragma.auto_vacuum() == 2);
}
