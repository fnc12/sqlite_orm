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
        
        template<class T, class ...Wargs>
        struct update_all_t;
        
        template<class ...Args, class ...Wargs>
        struct update_all_t<set_t<Args...>, Wargs...> {
            using set_type = set_t<Args...>;
            using conditions_type = std::tuple<Wargs...>;
            
            set_type set;
            conditions_type conditions;
        };
        
        template<class T, class ...Args>
        struct remove_all_t {
            using type = T;
            
            using conditions_type = std::tuple<Args...>;
            
            conditions_type conditions;
        };
    }
    
    template<class T, class ...Args>
    internal::remove_all_t<T, Args...> remove_all(Args ...args) {
        std::tuple<Args...> conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }
    
    template<class T, class ...Args>
    internal::get_all_t<T, Args...> get_all(Args ...args) {
        std::tuple<Args...> conditions{std::forward<Args>(args)...};
        return {move(conditions)};
    }
    
    template<class ...Args, class ...Wargs>
    internal::update_all_t<internal::set_t<Args...>, Wargs...> update_all(internal::set_t<Args...> set, Wargs ...wh) {
        std::tuple<Wargs...> conditions{std::forward<Wargs>(wh)...};
        return {std::move(set), move(conditions)};
    }
}
