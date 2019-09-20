#pragma once

#include <sqlite3.h>
#include <iterator> //  std::iterator_traits

#include "connection_holder.h"
#include "select_constraints.h"

namespace sqlite_orm {
    
    namespace internal {
        
        struct prepared_statement_base {
            sqlite3_stmt *stmt = nullptr;
            connection_ref con;
            
            ~prepared_statement_base() {
                if(this->stmt){
                    sqlite3_finalize(this->stmt);
                    this->stmt = nullptr;
                }
            }
        };
        
        template<class T>
        struct prepared_statement_t : prepared_statement_base {
            using expression_type = T;
            
            expression_type t;
            
            prepared_statement_t(T t_, sqlite3_stmt *stmt, connection_ref con_) :
            prepared_statement_base{stmt, std::move(con_)},
            t(std::move(t_))
            {}
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
        
        template<class T, class ...Ids>
        struct get_t {
            using type = T;
            
            using ids_type = std::tuple<Ids...>;
            
            ids_type ids;
        };
        
        template<class T, class ...Ids>
        struct get_pointer_t {
            using type = T;
            
            using ids_type = std::tuple<Ids...>;
            
            ids_type ids;
        };
        
        template<class T>
        struct update_t {
            using type = T;
            
            const type &obj;
            
            update_t(decltype(obj) obj_) : obj(obj_) {}
        };
        
        template<class T, class ...Ids>
        struct remove_t {
            using type = T;
            
            using ids_type = std::tuple<Ids...>;
            
            ids_type ids;
        };
        
        template<class T>
        struct insert_t {
            using type = T;
            
            const type &obj;
            
            insert_t(decltype(obj) obj_) : obj(obj_) {}
        };
        
        template<class T, class ...Cols>
        struct insert_explicit {
            using type = T;
            using columns_type = columns_t<Cols...>;
            
            const type &obj;
            columns_type columns;
            
            insert_explicit(decltype(obj) obj_, decltype(columns) columns_) : obj(obj_), columns(std::move(columns_)) {}
        };
        
        template<class T>
        struct replace_t {
            using type = T;
            
            const type &obj;
            
            replace_t(decltype(obj) obj_) : obj(obj_) {}
        };
        
        template<class It>
        struct insert_range_t {
            using iterator_type = It;
            using object_type = typename std::iterator_traits<iterator_type>::value_type;
            
            iterator_type from;
            iterator_type to;
        };
        
        template<class It>
        struct replace_range_t {
            using iterator_type = It;
            using object_type = typename std::iterator_traits<iterator_type>::value_type;
            
            iterator_type from;
            iterator_type to;
        };
    }
    
    template<class It>
    internal::replace_range_t<It> replace_range(It from, It to) {
        return {std::move(from), std::move(to)};
    }
    
    template<class It>
    internal::insert_range_t<It> insert_range(It from, It to) {
        return {std::move(from), std::move(to)};
    }
    
    template<class T>
    internal::replace_t<T> replace(const T &obj) {
        return {obj};
    }
    
    template<class T>
    internal::insert_t<T> insert(const T &obj) {
        return {obj};
    }
    
    template<class T, class ...Cols>
    internal::insert_explicit<T, Cols...> insert(const T &obj, internal::columns_t<Cols...> cols) {
        return {obj, std::move(cols)};
    }
    
    template<class T, class ...Ids>
    internal::remove_t<T, Ids...> remove(Ids ...ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }
    
    template<class T>
    internal::update_t<T> update(const T &obj) {
        return {obj};
    }
    
    template<class T, class ...Ids>
    internal::get_t<T, Ids...> get(Ids ...ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }
    
    template<class T, class ...Ids>
    internal::get_pointer_t<T, Ids...> get_pointer(Ids ...ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
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
