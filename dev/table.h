#pragma once

#include <string>   //  std::string
#include <type_traits>  //  std::remove_reference, std::is_same, std::is_base_of
#include <vector>   //  std::vector
#include <tuple>    //  std::tuple_size, std::tuple_element
#include <algorithm>    //  std::reverse, std::find_if

#include "column_result.h"
#include "static_magic.h"
#include "typed_comparator.h"
#include "constraints.h"
#include "tuple_helper.h"
#include "table_info.h"
#include "type_printer.h"
#include "column.h"

namespace sqlite_orm {
    
    namespace internal {
        
        struct table_base {
            
            /**
             *  Table name.
             */
            const std::string name;
            
            bool _without_rowid = false;
        };
        
        /**
         *  Table interface class. Implementation is hidden in `table_impl` class.
         */
        template<class T, class ...Cs>
        struct table_t : table_base {
            using object_type = T;
            using columns_type = std::tuple<Cs...>;
            
            static constexpr const int columns_count = static_cast<int>(std::tuple_size<columns_type>::value);
            
            columns_type columns;
            
            table_t(decltype(name) name_, columns_type columns_): table_base{std::move(name_)}, columns(std::move(columns_)) {}
            
            table_t<T, Cs...> without_rowid() const {
                auto res = *this;
                res._without_rowid = true;
                return res;
            }
            
            /**
             *  Function used to get field value from object by mapped member pointer/setter/getter
             */
            template<class F, class C>
            const F* get_object_field_pointer(const object_type &obj, C c) const {
                const F *res = nullptr;
                this->for_each_column_with_field_type<F>([&res, &c, &obj](auto &col){
                    using column_type = typename std::remove_reference<decltype(col)>::type;
                    using member_pointer_t = typename column_type::member_pointer_t;
                    using getter_type = typename column_type::getter_type;
                    using setter_type = typename column_type::setter_type;
                    // Make static_if have at least one input as a workaround for GCC bug:
                    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64095
                    if(!res){
                        static_if<std::is_same<C, member_pointer_t>{}>([&res, &obj, &col](const C &c){
                            if(compare_any(col.member_pointer, c)){
                                res = &(obj.*col.member_pointer);
                            }
                        })(c);
                    }
                    if(!res){
                        static_if<std::is_same<C, getter_type>{}>([&res, &obj, &col](const C &c){
                            if(compare_any(col.getter, c)){
                                res = &((obj).*(col.getter))();
                            }
                        })(c);
                    }
                    if(!res){
                        static_if<std::is_same<C, setter_type>{}>([&res, &obj, &col](const C &c){
                            if(compare_any(col.setter, c)){
                                res = &((obj).*(col.getter))();
                            }
                        })(c);
                    }
                });
                return res;
            }
            
            /**
             *  @return vector of column names of table.
             */
            std::vector<std::string> column_names() const {
                std::vector<std::string> res;
                this->for_each_column([&res](auto &c){
                    res.push_back(c.name);
                });
                return res;
            }
            
            /**
             *  Calls **l** with every primary key dedicated constraint
             */
            template<class L>
            void for_each_primary_key(const L &l) const {
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<internal::is_primary_key<column_type>{}>(l)(column);
                });
            }
            
            std::vector<std::string> composite_key_columns_names() const {
                std::vector<std::string> res;
                this->for_each_primary_key([this, &res](auto &c){
                    res = this->composite_key_columns_names(c);
                });
                return res;
            }
            
            std::vector<std::string> primary_key_column_names() const {
                std::vector<std::string> res;
                this->for_each_column_with<constraints::primary_key_t<>>([&res](auto &c){
                    res.push_back(c.name);
                });
                if(!res.size()){
                    res = this->composite_key_columns_names();
                }
                return res;
            }
            
            template<class ...Args>
            std::vector<std::string> composite_key_columns_names(const constraints::primary_key_t<Args...> &pk) const {
                std::vector<std::string> res;
                using pk_columns_tuple = decltype(pk.columns);
                res.reserve(std::tuple_size<pk_columns_tuple>::value);
                iterate_tuple(pk.columns, [this, &res](auto &v){
                    res.push_back(this->find_column_name(v));
                });
                return res;
            }
            
            /**
             *  Searches column name by class member pointer passed as first argument.
             *  @return column name or empty string if nothing found.
             */
            template<
            class F,
            class O,
            typename = typename std::enable_if<std::is_member_pointer<F O::*>::value && !std::is_member_function_pointer<F O::*>::value>::type>
            std::string find_column_name(F O::*m) const {
                std::string res;
                this->template for_each_column_with_field_type<F>([&res, m](auto c) {
                    if(c.member_pointer == m) {
                        res = c.name;
                    }
                });
                return res;
            }
            
            /**
             *  Searches column name by class getter function member pointer passed as first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class G>
            std::string find_column_name(G getter, typename std::enable_if<is_getter<G>::value>::type * = nullptr) const {
                std::string res;
                using field_type = typename getter_traits<G>::field_type;
                this->template for_each_column_with_field_type<field_type>([&res, getter](auto c) {
                    if(compare_any(c.getter, getter)) {
                        res = c.name;
                    }
                });
                return res;
            }
            
            /**
             *  Searches column name by class setter function member pointer passed as first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class S>
            std::string find_column_name(S setter, typename std::enable_if<is_setter<S>::value>::type * = nullptr) const {
                std::string res;
                using field_type = typename setter_traits<S>::field_type;
                this->template for_each_column_with_field_type<field_type>([&res, setter](auto c) {
                    if(compare_any(c.setter, setter)) {
                        res = c.name;
                    }
                });
                return res;
            }
            
            /**
             *  @return vector of column names that have constraints provided as template arguments (not_null, autoincrement).
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
             *  Iterates all columns and fires passed lambda. Lambda must have one and only templated argument Otherwise code will
             *  not compile. Excludes table constraints (e.g. foreign_key_t) at the end of the columns list. To iterate columns with
             *  table constraints use for_each_column_with_constraints instead.
             *  L is lambda type. Do not specify it explicitly.
             *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class L>
            void for_each_column(const L &l) const {
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<internal::is_column<column_type>{}>(l)(column);
                });
            }
            
            template<class L>
            void for_each_column_with_constraints(const L &l) const {
                iterate_tuple(this->columns, l);
            }
            
            template<class F, class L>
            void for_each_column_with_field_type(const L &l) const {
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<std::is_same<F, typename column_type::field_type>{}>(l)(column);
                });
            }
            
            /**
             *  Iterates all columns that have specified constraints and fires passed lambda.
             *  Lambda must have one and only templated argument Otherwise code will not compile.
             *  L is lambda type. Do not specify it explicitly.
             *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class Op, class L>
            void for_each_column_with(const L &l) const {
                using tuple_helper::tuple_contains_type;
                iterate_tuple(this->columns, [&l](auto &column){
                    using column_type = typename std::decay<decltype(column)>::type;
                    static_if<tuple_contains_type<Op, typename column_type::constraints_type>{}>(l)(column);
                });
            }
            
            std::vector<table_info> get_table_info() {
                std::vector<table_info> res;
                res.reserve(size_t(this->columns_count));
                this->for_each_column([&res](auto &col){
                    std::string dft;
                    using field_type = typename std::decay<decltype(col)>::type::field_type;
                    if(auto d = col.default_value()) {
                        auto needQuotes = std::is_base_of<text_printer, type_printer<field_type>>::value;
                        if(needQuotes){
                            dft = "'" + *d + "'";
                        }else{
                            dft = *d;
                        }
                    }
                    table_info i{
                        -1,
                        col.name,
                        type_printer<field_type>().print(),
                        col.not_null(),
                        dft,
                        col.template has<constraints::primary_key_t<>>(),
                    };
                    res.emplace_back(i);
                });
                auto compositeKeyColumnNames = this->composite_key_columns_names();
                for(size_t i = 0; i < compositeKeyColumnNames.size(); ++i) {
                    auto &columnName = compositeKeyColumnNames[i];
                    auto it = std::find_if(res.begin(),
                                           res.end(),
                                           [&columnName](const table_info &ti) {
                                               return ti.name == columnName;
                                           });
                    if(it != res.end()){
                        it->pk = static_cast<int>(i + 1);
                    }
                }
                return res;
            }
            
        };
    }
    
    /**
     *  Function used for table creation. Do not use table constructor - use this function
     *  cause table class is templated and its constructing too (just like std::make_unique or std::make_pair).
     */
    template<class ...Cs, class T = typename std::tuple_element<0, std::tuple<Cs...>>::type::object_type>
    internal::table_t<T, Cs...> make_table(const std::string &name, Cs ...args) {
        return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)};
    }
    
    template<class T, class ...Cs>
    internal::table_t<T, Cs...> make_table(const std::string &name, Cs ...args) {
        return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)};
    }
}
