#pragma once

#include <tuple>    //  std::tuple
#include <type_traits>  //  std::false_type, std::true_type
#include <utility>  //  std::index_sequence, std::index_sequence_for

namespace sqlite_orm {
    
    //  got from here http://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
    namespace tuple_helper {
        
        template <typename T, typename Tuple>
        struct has_type;
        
        template <typename T>
        struct has_type<T, std::tuple<>> : std::false_type {};
        
        template <typename T, typename U, typename... Ts>
        struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>> {};
        
        template <typename T, typename... Ts>
        struct has_type<T, std::tuple<T, Ts...>> : std::true_type {};
        
        template <typename T, typename Tuple>
        using tuple_contains_type = typename has_type<T, Tuple>::type;
        
        template<size_t N, class ...Args>
        struct iterator {
            
            template<class L>
            void operator()(const std::tuple<Args...> &t, L l, bool reverse = true) {
                if(reverse){
                    l(std::get<N>(t));
                    iterator<N - 1, Args...>()(t, l, reverse);
                }else{
                    iterator<N - 1, Args...>()(t, l, reverse);
                    l(std::get<N>(t));
                }
            }
        };
        
        template<class ...Args>
        struct iterator<0, Args...>{
            
            template<class L>
            void operator()(const std::tuple<Args...> &t, L l, bool /*reverse*/ = true) {
                l(std::get<0>(t));
            }
        };
        
        template<size_t N>
        struct iterator<N> {
            
            template<class L>
            void operator()(const std::tuple<> &, L, bool /*reverse*/ = true) {
                //..
            }
        };
    }
    
    namespace internal {
        
        template<class L, class ...Args>
        void iterate_tuple(const std::tuple<Args...> &t, const L &l) {
            using tuple_type = std::tuple<Args...>;
            tuple_helper::iterator<std::tuple_size<tuple_type>::value - 1, Args...>()(t, l, false);
        }
    }
}
