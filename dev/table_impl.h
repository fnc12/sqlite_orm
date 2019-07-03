#pragma once

#include <vector>   //  std::vector
#include <string>   //  std::string
#include <tuple>    //  std::tuple, std::tuple_size
#include <type_traits>  //  std::is_same, std::integral_constant, std::true_type, std::false_type

#include "column.h"
#include "tuple_helper.h"
#include "constraints.h"
#include "static_magic.h"

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  Regular table_impl class.
         */
        template<class... Cs>
        struct table_impl {
            using columns_type = std::tuple<Cs...>;
            
            columns_type columns;
            
            table_impl(Cs ...columns_) : columns{std::forward<Cs>(columns_)...} {}
            
            static constexpr const int columns_count = static_cast<int>(std::tuple_size<columns_type>::value);
            
            /**
             *  column_names_with implementation.
             *  @return vector of column names that have specified Op... constraints.
             */
            template<class ...Op>
            std::vector<std::string> column_names_with() {
                std::vector<std::string> res;
                iterate_tuple(this->columns, [&res](auto &column){
                    if(column.template has_every<Op...>()) {
                        res.emplace_back(column.name);
                    }
                });
                return res;
            }
            
            /**
             *  For each implementation. Calls templated lambda with every column not dedicated constraint
             */
            template<class L>
            void for_each_column(const L &l){
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<internal::is_column<column_type>{}>(l)(column);
                });
            }
            
            /**
             *  For each implementation. Calls templated lambda with every column
             *  and dedicated constraint
             */
            template<class L>
            void for_each_column_with_constraints(const L &l){
                iterate_tuple(this->columns, l);
            }
            
            /**
             *  For each implementation with a specific field type. Calls **l** only
             *  for columns with field_type equal F
             */
            template<class F, class L>
            void for_each_column_with_field_type(const L &l) {
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<std::is_same<F, typename column_type::field_type>{}>(l)(column);
                });
            }
            
            /**
             *  Calls **l** for every column with specified constraint Op
             */
            template<class Op, class L>
            void for_each_column_with(const L &l) {
                using tuple_helper::tuple_contains_type;
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<tuple_contains_type<Op, typename column_type::constraints_type>{}>(l)(column);
                });
            }
            
            /**
             *  Calls **l** with every primary key dedicated constraint
             */
            template<class L>
            void for_each_primary_key(const L &l) {
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<internal::is_primary_key<column_type>{}>(l)(column);
                });
            }
        };
    }
}
