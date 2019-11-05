#pragma once

#include <type_traits>  //  std::decay
#include <functional>   //  std::reference_wrapper

#include "prepared_statement.h"

namespace sqlite_orm {
    namespace internal {
        
        template<class T>
        struct expression_object_type;
        
        template<class T>
        struct expression_object_type<replace_t<T>> {
            using type = typename std::decay<T>::type;
        };
        
        template<class T>
        struct expression_object_type<replace_t<std::reference_wrapper<T>>> {
            using type = typename std::decay<T>::type;
        };
        
        template<class T>
        struct get_object_t;
        
        template<class T>
        struct get_object_t<const T> : get_object_t<T> {};
        
        template<class T>
        auto &get_object(T &t) {
            get_object_t<T> obj;
            return obj(t);
        }
        
        template<class T>
        struct get_object_t<replace_t<T>> {
            using expression_type = replace_t<T>;
            
            template<class O>
            auto &operator()(O &e) const {
                return e.obj;
            }
        };
        
        template<class T>
        struct get_object_t<replace_t<const T>> {
            using expression_type = replace_t<const T>;
            
            template<class O>
            auto &operator()(O &e) const {
                return e.obj;
            }
        };
        
        template<class T>
        struct get_object_t<replace_t<std::reference_wrapper<T>>> {
            using expression_type = replace_t<std::reference_wrapper<T>>;
            
            T &operator()(expression_type &e) const {
                return e.obj.get();
            }
        };
        
        template<class T>
        struct get_object_t<replace_t<std::reference_wrapper<const T>>> {
            using expression_type = replace_t<std::reference_wrapper<const T>>;
            
            const T &operator()(const expression_type &e) const {
                return e.obj.get();
            }
        };
    }
}
