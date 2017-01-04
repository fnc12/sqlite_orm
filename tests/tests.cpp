//
//  tests.cpp
//  CPPTest
//
//  Created by John Zakharov on 05.01.17.
//  Copyright © 2017 John Zakharov. All rights reserved.
//

//#include "tests.hpp"

#include "sqlite_orm.h"

#include <cassert>

using namespace sqlite_orm;

int main() {
    
    //  int
    assert(*to_sqlite_type("INT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("INTEGER") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("TINYINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("SMALLINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("MEDIUMINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("BIGINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("UNSIGNED BIG INT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("INT2") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("INT8") == sqlite_type::INTEGER);
    
    //  text
    assert(*to_sqlite_type("TEXT") == sqlite_type::TEXT);
    assert(*to_sqlite_type("CLOB") == sqlite_type::TEXT);
//    assert(*to_sqlite_type("CHARACTER()") == sqlite_type::TEXT);
    for(auto i = 0; i< 255; ++i) {
//        auto sqliteTypeString = "CHARACTER(" + std::to_string(i) + ")";
        assert(*to_sqlite_type("CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("VARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("VARYING CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("NCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("NATIVE CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("NVARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
    }
    
    //  blob..
    assert(*to_sqlite_type("BLOB") == sqlite_type::BLOB);
    
    //  real
    assert(*to_sqlite_type("REAL") == sqlite_type::REAL);
    assert(*to_sqlite_type("DOUBLE") == sqlite_type::REAL);
    assert(*to_sqlite_type("DOUBLE PRECISION") == sqlite_type::REAL);
    assert(*to_sqlite_type("FLOAT") == sqlite_type::REAL);
    
    assert(*to_sqlite_type("NUMERIC") == sqlite_type::REAL);
    for(auto i = 0; i < 255; ++i) {
        for(auto j = 0; j < 10; ++j) {
            assert(*to_sqlite_type("DECIMAL(" + std::to_string(i) + "," + std::to_string(j) + ")") == sqlite_type::REAL);
        }
    }
    assert(*to_sqlite_type("BOOLEAN") == sqlite_type::REAL);
    assert(*to_sqlite_type("DATE") == sqlite_type::REAL);
    assert(*to_sqlite_type("DATETIME") == sqlite_type::REAL);
    
    
//    assert(false);
}
