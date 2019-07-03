#include <sqlite_orm/sqlite_orm.h>
#include <iostream> //  std::cout, std::endl
#include <cstdio>   //  ::remove

using namespace sqlite_orm;

using std::cout;
using std::endl;

void testJournalMode() {
    cout << __func__ << endl;
    
    auto filename = "journal_mode.sqlite";
    ::remove(filename);
    auto storage = make_storage(filename);
    auto jm = storage.pragma.journal_mode();
    assert(jm == decltype(jm)::DELETE);
    
    for(auto i = 0; i < 2; ++i){
        if(i == 0) {
            storage.begin_transaction();
        }
        storage.pragma.journal_mode(journal_mode::MEMORY);
        jm = storage.pragma.journal_mode();
        assert(jm == decltype(jm)::MEMORY);
        
        if(i == 1) {    //  WAL cannot be set within a transaction
            storage.pragma.journal_mode(journal_mode::WAL);
            jm = storage.pragma.journal_mode();
            assert(jm == decltype(jm)::WAL);
        }
        
        storage.pragma.journal_mode(journal_mode::OFF);
        jm = storage.pragma.journal_mode();
        assert(jm == decltype(jm)::OFF);
        
        storage.pragma.journal_mode(journal_mode::PERSIST);
        jm = storage.pragma.journal_mode();
        assert(jm == decltype(jm)::PERSIST);
        
        storage.pragma.journal_mode(journal_mode::TRUNCATE);
        jm = storage.pragma.journal_mode();
        assert(jm == decltype(jm)::TRUNCATE);
        
        if(i == 0) {
            storage.rollback();
        }
    }
}

void testSynchronous() {
    cout << __func__ << endl;
    
    auto storage = make_storage("");
    const auto value = 1;
    storage.pragma.synchronous(value);
    assert(storage.pragma.synchronous() == value);
    
    storage.begin_transaction();
    
    const auto newValue = 2;
    try{
        storage.pragma.synchronous(newValue);
        throw std::runtime_error("Must not fire");
    }catch(const std::system_error&) {
        //  Safety level may not be changed inside a transaction
        assert(storage.pragma.synchronous() == value);
    }
    
    storage.commit();
}

void testUserVersion() {
    cout << __func__ << endl;
    
    auto storage = make_storage("");
    auto version = storage.pragma.user_version();
    
    storage.pragma.user_version(version + 1);
    assert(storage.pragma.user_version() == version + 1);
    
    storage.begin_transaction();
    storage.pragma.user_version(version + 2);
    assert(storage.pragma.user_version() == version + 2);
    storage.commit();
}

void testAutoVacuum() {
    cout << __func__ << endl;
    
    auto filename = "autovacuum.sqlite";
    ::remove(filename);
    
    auto storage = make_storage(filename);
    
    
    storage.pragma.auto_vacuum(0);
    assert(storage.pragma.auto_vacuum() == 0);
    
    storage.pragma.auto_vacuum(1);
    assert(storage.pragma.auto_vacuum() == 1);
    
    storage.pragma.auto_vacuum(2);
    assert(storage.pragma.auto_vacuum() == 2);
}

int main() {
    
    testUserVersion();
    
    testSynchronous();
    
    testJournalMode();
    
    testAutoVacuum();
    
    return 0;
}
