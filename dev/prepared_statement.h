#pragma once

#include <sqlite3.h>
#include <iterator> //  std::iterator_traits
#include <string>   //  std::string
#include <type_traits>  //  std::true_type, std::false_type

#include "connection_holder.h"
#include "select_constraints.h"

namespace sqlite_orm {
    
    template<class T>
    struct by_val {
        using type = T;
        
        type obj;
    };
    
    namespace internal {
        
        template<class T>
        struct is_by_val : std::false_type {};
        
        template<class T>
        struct is_by_val<by_val<T>> : std::true_type {};
        
        struct prepared_statement_base {
            sqlite3_stmt *stmt = nullptr;
            connection_ref con;
            
            ~prepared_statement_base() {
                if(this->stmt){
                    sqlite3_finalize(this->stmt);
                    this->stmt = nullptr;
                }
            }
            
            std::string sql() const {
                if(this->stmt){
                    if(auto res = sqlite3_sql(this->stmt)){
                        return res;
                    }else{
                        return {};
                    }
                }else{
                    return {};
                }
            }
            
            std::string expanded_sql() const {
                if(this->stmt){
                    if(auto res = sqlite3_expanded_sql(this->stmt)){
                        std::string result = res;
                        sqlite3_free(res);
                        return result;
                    }else{
                        return {};
                    }
                }else{
                    return {};
                }
            }
#if SQLITE_VERSION_NUMBER >= 3027000
            std::string normalized_sql() const {
                if(this->stmt){
                    if(auto res = sqlite3_normalized_sql(this->stmt)){
                        return res;
                    }else{
                        return {};
                    }
                }else{
                    return {};
                }
            }
#endif
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
        
        template<class T, bool by_ref>
        struct update_t;
        
        template<class T>
        struct update_t<T, true>{
            using type = T;
            
            const type &obj;
            
            update_t(decltype(obj) obj_) : obj(obj_) {}
        };
        
        template<class T>
        struct update_t<T, false>{
            using type = T;
            
            type obj;
        };
        
        template<class T, class ...Ids>
        struct remove_t {
            using type = T;
            
            using ids_type = std::tuple<Ids...>;
            
            ids_type ids;
        };
        
        template<class T, bool by_ref>
        struct insert_t;
        
        template<class T>
        struct insert_t<T, true> {
            using type = T;
            
            const type &obj;
            
            insert_t(decltype(obj) obj_) : obj(obj_) {}
        };
        
        template<class T>
        struct insert_t<T, false> {
            using type = T;
            
            type obj;
        };
        
        template<class T, bool by_ref, class ...Cols>
        struct insert_explicit;
        
        template<class T, class ...Cols>
        struct insert_explicit<T, true, Cols...> {
            using type = T;
            using columns_type = columns_t<Cols...>;
            
            const type &obj;
            columns_type columns;
            
            insert_explicit(decltype(obj) obj_, decltype(columns) columns_) : obj(obj_), columns(std::move(columns_)) {}
        };
        
        template<class T, class ...Cols>
        struct insert_explicit<T, false, Cols...> {
            using type = T;
            using columns_type = columns_t<Cols...>;
            
            type obj;
            columns_type columns;
            
            insert_explicit(decltype(obj) obj_, decltype(columns) columns_) : obj(obj_), columns(std::move(columns_)) {}
        };
        
        template<class T, bool by_ref>
        struct replace_t;
        
        template<class T>
        struct replace_t<T, true> {
            using type = T;
            
            const type &obj;
            
            replace_t(decltype(obj) obj_) : obj(obj_) {}
        };
        
        template<class T>
        struct replace_t<T, false> {
            using type = T;
            
            type obj;
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
    internal::replace_t<T, true> replace(const T &obj) {
        static_assert(!internal::is_by_val<T>::value, "by_val is not allowed here");
        return {obj};
    }
    
    template<class B>
    internal::replace_t<typename B::type, false> replace(typename B::type obj) {
        static_assert(internal::is_by_val<B>::value, "by_val expected");
        return {std::move(obj)};
    }
    
    template<class T>
    internal::insert_t<T, true> insert(const T &obj) {
        static_assert(!internal::is_by_val<T>::value, "by_val is not allowed here");
        return {obj};
    }
    
    template<class B>
    internal::insert_t<typename B::type, false> insert(typename B::type obj) {
        static_assert(internal::is_by_val<B>::value, "by_val expected");
        return {std::move(obj)};
    }
    
    template<class T, class ...Cols>
    internal::insert_explicit<T, true, Cols...> insert(const T &obj, internal::columns_t<Cols...> cols) {
        static_assert(!internal::is_by_val<T>::value, "by_val is not allowed here");
        return {obj, std::move(cols)};
    }
    
    template<class B, class ...Cols>
    internal::insert_explicit<typename B::type, false, Cols...> insert(typename B::type obj, internal::columns_t<Cols...> cols) {
        static_assert(internal::is_by_val<B>::value, "by_val expected");
        return {std::move(obj), std::move(cols)};
    }
    
    template<class T, class ...Ids>
    internal::remove_t<T, Ids...> remove(Ids ...ids) {
        std::tuple<Ids...> idsTuple{std::forward<Ids>(ids)...};
        return {move(idsTuple)};
    }
    
    template<class T>
    internal::update_t<T, true> update(const T &obj) {
        static_assert(!internal::is_by_val<T>::value, "by_val is not allowed here");
        return {obj};
    }
    
    template<class B>
    internal::update_t<typename B::type, false> update(typename B::type obj) {
        static_assert(internal::is_by_val<B>::value, "by_val expected");
        return {std::move(obj)};
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
