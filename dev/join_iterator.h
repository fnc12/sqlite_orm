#pragma once

#include "conditions.h"

namespace sqlite_orm {
    
    namespace internal {
        
        template<class ...Args>
        struct join_iterator {
            
            template<class L>
            void operator()(const L &) {
                //..
            }
        };
        
        template<>
        struct join_iterator<> {
            
            template<class L>
            void operator()(const L &) {
                //..
            }
        };
        
        template<class H, class ...Tail>
        struct join_iterator<H, Tail...> : public join_iterator<Tail...>{
            using super = join_iterator<Tail...>;
            
            template<class L>
            void operator()(const L &l) {
                this->super::operator()(l);
            }
            
        };
        
        template<class T, class ...Tail>
        struct join_iterator<conditions::cross_join_t<T>, Tail...> : public join_iterator<Tail...>{
            using super = join_iterator<Tail...>;
            using join_type = conditions::cross_join_t<T>;
            
            template<class L>
            void operator()(const L &l) {
                l(*this);
                this->super::operator()(l);
            }
        };
        
        template<class T, class ...Tail>
        struct join_iterator<conditions::natural_join_t<T>, Tail...> : public join_iterator<Tail...>{
            using super = join_iterator<Tail...>;
            using join_type = conditions::natural_join_t<T>;
            
            template<class L>
            void operator()(const L &l) {
                l(*this);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::left_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = conditions::left_join_t<T, O>;
            
            template<class L>
            void operator()(const L &l) {
                l(*this);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = conditions::join_t<T, O>;
            
            template<class L>
            void operator()(const L &l) {
                l(*this);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::left_outer_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = conditions::left_outer_join_t<T, O>;
            
            template<class L>
            void operator()(const L &l) {
                l(*this);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::inner_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = conditions::inner_join_t<T, O>;
            
            template<class L>
            void operator()(const L &l) {
                l(*this);
                this->super::operator()(l);
            }
        };
    }
}
