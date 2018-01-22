
#ifndef table_h
#define table_h

#include <string>
#include <vector>
#include <tuple>
#include <type_traits>
#include <algorithm>

#include "column.h"
#include "constraints/constraints.h"

namespace sqlite_orm {
    
    namespace internal {
        
        struct table_info {
            int cid;
            std::string name;
            std::string type;
            bool notnull;
            std::string dflt_value;
            int pk;
        };
        
        /**
         *  Common case for table_impl class.
         */
        template<typename... Args>
        struct table_impl {
            
            std::vector<std::string> column_names() { return {}; }
            
            template<class ...Op>
            std::vector<std::string> column_names_exept() { return {}; }
            
            template<class ...Op>
            std::vector<std::string> column_names_with() { return {}; }
            
            template<class L>
            void for_each_column(L) {}
            
            template<class L>
            void for_each_column_with_constraints(L) {}
            
            template<class F, class L>
            void for_each_column_with_field_type(L) {}
            
            template<class Op, class L>
            void for_each_column_exept(L){}
            
            template<class Op, class L>
            void for_each_column_with(L) {}
            
            template<class L>
            void for_each_primary_key(L) {}
            
            int columns_count() const {
                return 0;
            }
            
        };
        
        template<typename H, typename... T>
        struct table_impl<H, T...> : private table_impl<T...> {
            using column_type = H;
            using tail_types = std::tuple<T...>;
            
            table_impl(H h, T ...t) : super(t...), col(h) {}
            
            column_type col;
            
            int columns_count() const {
                return 1 + this->super::columns_count();
            }
            
            /**
             *  column_names_with implementation. Notice that result will be reversed.
             *  It is reversed back in `table` class.
             *  @return vector of column names that have specified Op... conditions.
             */
            template<class ...Op>
            std::vector<std::string> column_names_with() {
                auto res = this->super::template column_names_with<Op...>();
                if(this->col.template has_every<Op...>()) {
                    res.emplace_back(this->col.name);
                }
                return res;
            }
            
            /**
             *  For each implementation. Calls templated lambda with its column
             *  and passed call to superclass.
             */
            template<class L>
            void for_each_column(L l){
                this->apply_to_col_if(l, internal::is_column<column_type>{});
                this->super::for_each_column(l);
            }
            
            /**
             *  For each implementation. Calls templated lambda with its column
             *  and passed call to superclass.
             */
            template<class L>
            void for_each_column_with_constraints(L l){
                l(this->col);
                this->super::for_each_column_with_constraints(l);
            }
            
            template<class F, class L>
            void for_each_column_with_field_type(L l) {
                this->apply_to_col_if(l, std::is_same<F, typename column_type::field_type>{});
                this->super::template for_each_column_with_field_type<F, L>(l);
            }
            
            /**
             *  Working version of `for_each_column_exept`. Calls lambda if column has no option and fire super's function.
             */
            template<class Op, class L>
            void for_each_column_exept(L l) {
                using has_opt = tuple_helper::tuple_contains_type<Op, typename column_type::constraints_type>;
                this->apply_to_col_if(l, std::integral_constant<bool, !has_opt::value>{});
                this->super::template for_each_column_exept<Op, L>(l);
            }
            
            /**
             *  Working version of `for_each_column_with`. Calls lambda if column has option and fire super's function.
             */
            template<class Op, class L>
            void for_each_column_with(L l) {
                this->apply_to_col_if(l, tuple_helper::tuple_contains_type<Op, typename column_type::constraints_type>{});
                this->super::template for_each_column_with<Op, L>(l);
            }
            
            /**
             *  Calls l(this->col) if H is primary_key_t
             */
            template<class L>
            void for_each_primary_key(L l) {
                this->apply_to_col_if(l, internal::is_primary_key<H>{});
                this->super::for_each_primary_key(l);
            }
            
        protected:
            
            template<class L>
            void apply_to_col_if(L& l, std::true_type) {
                l(this->col);
            }
            
            template<class L>
            void apply_to_col_if(L&, std::false_type) {}
            
        private:
            using super = table_impl<T...>;
        };
        
        /**
         *  Table interface class. Implementation is hidden in `table_impl` class.
         */
        template<class ...Cs>
        struct table_t {
            using impl_type = table_impl<Cs...>;
            using object_type = typename std::tuple_element<0, std::tuple<Cs...>>::type::object_type;
            
            /**
             *  Table name.
             */
            const std::string name;
            
            /**
             *  Implementation that stores columns information.
             */
            impl_type impl;
            
            table_t(decltype(name) name_, decltype(impl) impl_):name(std::move(name_)), impl(std::move(impl_)){}
            
            bool _without_rowid = false;
            
            table_t<Cs...> without_rowid() const {
                auto res = *this;
                res._without_rowid = true;
                return res;
            }
            
            /**
             *  @return vector of column names of table.
             */
            std::vector<std::string> column_names() {
                std::vector<std::string> res;
                this->impl.for_each_column([&res](auto &c){
                    res.push_back(c.name);
                });
                return res;
            }
            
            std::vector<std::string> composite_key_columns_names() {
                std::vector<std::string> res;
                this->impl.for_each_primary_key([this, &res](auto c){
                    res = this->composite_key_columns_names(c);
                });
                return res;
            }
            
            std::vector<std::string> primary_key_column_names() {
                std::vector<std::string> res;
                this->impl.template for_each_column_with<internal::constraints::primary_key_t<>>([&res](auto &c){
                    res.push_back(c.name);
                });
                if(!res.size()){
                    res = this->composite_key_columns_names();
                }
                return res;
            }
            
            template<class ...Args>
            std::vector<std::string> composite_key_columns_names(internal::constraints::primary_key_t<Args...> pk) {
                std::vector<std::string> res;
                using pk_columns_tuple = decltype(pk.columns);
                res.reserve(std::tuple_size<pk_columns_tuple>::value);
                tuple_helper::iterator<std::tuple_size<pk_columns_tuple>::value - 1, Args...>()(pk.columns, [this, &res](auto &v){
                    res.push_back(this->find_column_name(v));
                });
                return res;
            }
            
            int columns_count() const {
                return this->impl.columns_count();
            }
            
            /**
             *  Searches column name by class member pointer passed as first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class F, class O>
            std::string find_column_name(F O::*m) {
                std::string res;
                this->template for_each_column_with_field_type<F>([&](auto c) {
                    if(c.member_pointer == m) {
                        res = c.name;
                    }
                });
                return res;
            }
            
            template<class F, class O>
            std::string find_column_name(const F& (O::*getter)() const) {
                std::string res;
                this->template for_each_column_with_field_type<F>([&](auto c) {
                    if(c.getter == getter) {
                        res = c.name;
                    }
                });
                return res;
            }
            
            template<class F, class O>
            std::string find_column_name(void (O::*setter)(F)) {
                std::string res;
                this->template for_each_column_with_field_type<F>([&](auto c) {
                    if(c.setter == setter) {
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
                auto res = this->impl.template column_names_with<Op...>();
                std::reverse(res.begin(),
                             res.end());
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
            void for_each_column(L l) {
                this->impl.for_each_column(l);
            }
            
            template<class L>
            void for_each_column_with_constraints(L l) {
                this->impl.for_each_column_with_constraints(l);
            }
            
            template<class F, class L>
            void for_each_column_with_field_type(L l) {
                this->impl.template for_each_column_with_field_type<F, L>(l);
            }
            
            /**
             *  Iterates all columns exept ones that have specified constraints and fires passed lambda.
             *  Lambda must have one and only templated argument Otherwise code will not compile.
             *  L is lambda type. Do not specify it explicitly.
             *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class Op, class L>
            void for_each_column_exept(L l) {
                this->impl.template for_each_column_exept<Op>(l);
            }
            
            /**
             *  Iterates all columns that have specified constraints and fires passed lambda.
             *  Lambda must have one and only templated argument Otherwise code will not compile.
             *  L is lambda type. Do not specify it explicitly.
             *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class Op, class L>
            void for_each_column_with(L l) {
                this->impl.template for_each_column_with<Op>(l);
            }
            
            std::vector<table_info> get_table_info() {
                std::vector<table_info> res;
                res.reserve(size_t(this->columns_count()));
                this->for_each_column([&res](auto col){
                    std::string dft;
                    if(auto d = col.default_value()) {
                        using field_type = typename decltype(col)::field_type;
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
                        type_printer<typename decltype(col)::field_type>().print(),
                        col.not_null(),
                        dft,
                        col.template has<internal::constraints::primary_key_t<>>(),
                    };
                    res.emplace_back(i);
                });
                std::vector<std::string> compositeKeyColumnNames;
                this->impl.for_each_primary_key([this, &compositeKeyColumnNames](auto c){
                    compositeKeyColumnNames = this->composite_key_columns_names(c);
                });
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
}

#endif /* table_h */
