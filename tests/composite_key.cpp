#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Composite key"){
    struct Record {
        int year = 0;
        int month = 0;
        int amount = 0;
    };
    
    auto recordsTableName = "records";
    auto storage = make_storage({},
                                make_table(recordsTableName,
                                           make_column("year", &Record::year),
                                           make_column("month", &Record::month),
                                           make_column("amount", &Record::amount),
                                           primary_key(&Record::year, &Record::month)));
    
    storage.sync_schema();
    REQUIRE(storage.sync_schema().at(recordsTableName) == sqlite_orm::sync_schema_result::already_in_sync);
    
    //  after #18
    SECTION("Repeat sync") {
        auto storage2 = make_storage("",
                                     make_table(recordsTableName,
                                                make_column("year", &Record::year),
                                                make_column("month", &Record::month),
                                                make_column("amount", &Record::amount),
                                                primary_key(&Record::month, &Record::year)));
        storage2.sync_schema();
        REQUIRE(storage2.sync_schema().at(recordsTableName) == sqlite_orm::sync_schema_result::already_in_sync);
        
        auto storage3 = make_storage("",
                                     make_table(recordsTableName,
                                                make_column("year", &Record::year),
                                                make_column("month", &Record::month),
                                                make_column("amount", &Record::amount),
                                                primary_key(&Record::amount, &Record::month, &Record::year)));
        storage3.sync_schema();
        REQUIRE(storage3.sync_schema().at(recordsTableName) == sqlite_orm::sync_schema_result::already_in_sync);
    }
    
    //  after #348
    SECTION("get & get_pointer") {
        storage.replace(Record{1, 2, 3});
        
        REQUIRE(storage.count<Record>() == 1);
        
        auto record = storage.get<Record>(1, 2);
        REQUIRE(record.year == 1);
        REQUIRE(record.month == 2);
        REQUIRE(record.amount == 3);
        
        auto recordPointer = storage.get_pointer<Record>(1, 2);
        REQUIRE(recordPointer);
    }
}
