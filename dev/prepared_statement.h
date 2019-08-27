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
        
        template<class T, class ...Args>
        struct get_all_t {
            using type = T;
            
            using conditions_type = std::tuple<Args...>;
            
            conditions_type conditions;
        };
    }
    
    template<class T, class ...Args>
    internal::get_all_t<T, Args...> get_all(Args ...args) {
        return {std::forward<Args>(args)...};
    }
}
