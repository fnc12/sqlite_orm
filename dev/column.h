#pragma once

#include <tuple>    //  std::tuple
#include <string>   //  std::string
#include <memory>   //  std::shared_ptr
#include <type_traits>  //  std::true_type, std::false_type

#include "type_is_nullable.h"
#include "tuple_helper.h"
#include "default_value_extractor.h"

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  This class stores single column info. column_t is a pair of [column_name:member_pointer] mapped to a storage
         *  O is a mapped class, e.g. User
         *  T is a mapped class'es field type, e.g. &User::name
         *  Op... is a constraints pack, e.g. primary_key_t, autoincrement_t etc
         */
        template<class O, class T, class ...Op>
        struct column_t {
            using object_type = O;
            using field_type = T;
            using constraints_type = std::tuple<Op...>;
            using member_pointer_t = field_type object_type::*;
            using getter_type = const field_type& (object_type::*)() const;
            using setter_type = void (object_type::*)(field_type);
            
            /**
             *  Column name. Specified during construction in `make_column`.
             */
            const std::string name;
            
            /**
             *  Member pointer used to read/write member
             */
            member_pointer_t member_pointer/* = nullptr*/;
            
            /**
             *  Getter member function pointer to get a value. If member_pointer is null than
             *  `getter` and `setter` must be not null
             */
            getter_type getter/* = nullptr*/;
            
            /**
             *  Setter member function
             */
            setter_type setter/* = nullptr*/;
            
            /**
             *  Constraints tuple
             */
            constraints_type constraints;
            
            /**
             *  Simplified interface for `NOT NULL` constraint
             */
            bool not_null() const {
                return !type_is_nullable<field_type>::value;
            }
            
            template<class Opt>
            constexpr bool has() const {
                return tuple_helper::tuple_contains_type<Opt, constraints_type>::value;
            }
            
            template<class O1, class O2, class ...Opts>
            constexpr bool has_every() const  {
                if(has<O1>() && has<O2>()) {
                    return true;
                }else{
                    return has_every<Opts...>();
                }
            }
            
            template<class O1>
            constexpr bool has_every() const {
                return has<O1>();
            }
            
            /**
             *  Simplified interface for `DEFAULT` constraint
             *  @return string representation of default value if it exists otherwise nullptr
             */
            std::shared_ptr<std::string> default_value() {
                std::shared_ptr<std::string> res;
                tuple_helper::iterator<std::tuple_size<constraints_type>::value - 1, Op...>()(constraints, [&res](auto &v){
                    auto dft = internal::default_value_extractor()(v);
                    if(dft){
                        res = dft;
                    }
                });
                return res;
            }
        };
        
        /**
         *  Column traits. Common case.
         */
        template<class T>
        struct is_column : public std::false_type {};
        
        /**
         *  Column traits. Specialized case case.
         */
        template<class O, class T, class ...Op>
        struct is_column<column_t<O, T, Op...>> : public std::true_type {};
        
    }
    
    /**
     *  Column builder function. You should use it to create columns instead of constructor
     */
    template<class O, class T, class ...Op>
    internal::column_t<O, T, Op...> make_column(const std::string &name, T O::*m, Op ...constraints){
        return {name, m, nullptr, nullptr, std::make_tuple(constraints...)};
    }
    
    /**
     *  Column builder function with setter and getter. You should use it to create columns instead of constructor
     */
    template<class O, class T, class ...Op>
    internal::column_t<O, T, Op...> make_column(const std::string &name, void (O::*setter)(T), const T& (O::*getter)() const, Op ...constraints) {
        return {name, nullptr, getter, setter, std::make_tuple(constraints...)};
    }
    
    /**
     *  Column builder function with getter and setter (reverse order). You should use it to create columns instead of constructor
     */
    template<class O, class T, class ...Op>
    internal::column_t<O, T, Op...> make_column(const std::string &name, const T& (O::*getter)() const, void (O::*setter)(T), Op ...constraints) {
        return {name, nullptr, getter, setter, std::make_tuple(constraints...)};
    }
    
}
