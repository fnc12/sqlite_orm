#pragma once

#include <sqlite3.h>

namespace sqlite_orm {
    
    namespace internal {
        
        struct prepared_statement_base {
            sqlite3_stmt *stmt = nullptr;
        };
        
        template<class T>
        struct prepared_statement_t : prepared_statement_base {
            T t;
            
            prepared_statement_t(T t_, sqlite3_stmt *stmt) : prepared_statement_base{stmt}, t(std::move(t_)) {}
        };
    }
}
