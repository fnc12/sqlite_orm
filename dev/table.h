#pragma once

#include <string>  //  std::string
#include <type_traits>  //  std::remove_reference, std::is_same, std::is_base_of
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_size, std::tuple_element
#include <algorithm>  //  std::reverse, std::find_if

#include "column_result.h"
#include "static_magic.h"
#include "typed_comparator.h"
#include "constraints.h"
#include "tuple_helper/tuple_helper.h"
#include "table_info.h"
#include "type_printer.h"
#include "column.h"

namespace sqlite_orm {

    namespace internal {

        struct basic_table {

            /**
             *  Table name.
             */
            std::string name;
        };

        /**
         *  Table class.
         */
        template<class T, bool WithoutRowId, class... Cs>
        struct table_t : basic_table {
            using super = basic_table;
            using object_type = T;
            using elements_type = std::tuple<Cs...>;

            static constexpr const int elements_count = static_cast<int>(std::tuple_size<elements_type>::value);
            static constexpr const bool is_without_rowid = WithoutRowId;

            elements_type elements;

            table_t(std::string name_, elements_type elements_) : super{move(name_)}, elements{move(elements_)} {}

            table_t<T, true, Cs...> without_rowid() const {
                return {this->name, this->elements};
            }

            /**
             *  Function used to get field value from object by mapped member pointer/setter/getter
             */
            template<class F, class C>
            const F* get_object_field_pointer(const object_type& obj, C c) const {
                const F* res = nullptr;
                this->for_each_column_with_field_type<F>([&res, &c, &obj](auto& col) {
                    using column_type = typename std::remove_reference<decltype(col)>::type;
                    using member_pointer_t = typename column_type::member_pointer_t;
                    using getter_type = typename column_type::getter_type;
                    using setter_type = typename column_type::setter_type;
                    // Make static_if have at least one input as a workaround for GCC bug:
                    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64095
                    if(!res) {
                        static_if<std::is_same<C, member_pointer_t>{}>([&res, &obj, &col](const C& c_) {
                            if(compare_any(col.member_pointer, c_)) {
                                res = &(obj.*col.member_pointer);
                            }
                        })(c);
                    }
                    if(!res) {
                        static_if<std::is_same<C, getter_type>{}>([&res, &obj, &col](const C& c_) {
                            if(compare_any(col.getter, c_)) {
                                res = &((obj).*(col.getter))();
                            }
                        })(c);
                    }
                    if(!res) {
                        static_if<std::is_same<C, setter_type>{}>([&res, &obj, &col](const C& c_) {
                            if(compare_any(col.setter, c_)) {
                                res = &((obj).*(col.getter))();
                            }
                        })(c);
                    }
                });
                return res;
            }

            template<class C>
            bool exists_in_composite_primary_key(const C& column) const {
                auto res = false;
                this->for_each_primary_key([&column, &res](auto& primaryKey) {
                    iterate_tuple(primaryKey.columns, [&res, &column](auto& value) {
                        if(!res) {
                            if(column.member_pointer) {
                                res = compare_any(value, column.member_pointer);
                            } else {
                                res = compare_any(value, column.getter) || compare_any(value, column.setter);
                            }
                        }
                    });
                });
                return res;
            }

            /**
             *  Calls **l** with every primary key dedicated constraint
             */
            template<class L>
            void for_each_primary_key(const L& lambda) const {
                iterate_tuple(this->elements, [&lambda](auto& element) {
                    using element_type = typename std::decay<decltype(element)>::type;
                    static_if<is_primary_key<element_type>{}>(lambda)(element);
                });
            }

            std::vector<std::string> composite_key_columns_names() const {
                std::vector<std::string> res;
                this->for_each_primary_key([this, &res](auto& c) {
                    res = this->composite_key_columns_names(c);
                });
                return res;
            }

            std::vector<std::string> primary_key_column_names() const {
                std::vector<std::string> res;
                this->for_each_column_with<primary_key_t<>>([&res](auto& c) {
                    res.push_back(c.name);
                });
                if(!res.size()) {
                    res = this->composite_key_columns_names();
                }
                return res;
            }

            template<class... Args>
            std::vector<std::string> composite_key_columns_names(const primary_key_t<Args...>& pk) const {
                std::vector<std::string> res;
                using pk_columns_tuple = decltype(pk.columns);
                res.reserve(std::tuple_size<pk_columns_tuple>::value);
                iterate_tuple(pk.columns, [this, &res](auto& v) {
                    if(auto columnName = this->find_column_name(v)) {
                        res.push_back(*columnName);
                    } else {
                        res.push_back({});
                    }
                });
                return res;
            }

            /**
             *  Searches column name by class member pointer passed as the first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class F,
                     class O,
                     typename = typename std::enable_if<std::is_member_pointer<F O::*>::value &&
                                                        !std::is_member_function_pointer<F O::*>::value>::type>
            const std::string* find_column_name(F O::*m) const {
                const std::string* res = nullptr;
                this->template for_each_column_with_field_type<F>([&res, m](auto& c) {
                    if(c.member_pointer == m) {
                        res = &c.name;
                    }
                });
                return res;
            }

            /**
             *  Searches column name by class getter function member pointer passed as first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class G>
            const std::string* find_column_name(G getter,
                                                typename std::enable_if<is_getter<G>::value>::type* = nullptr) const {
                const std::string* res = nullptr;
                using field_type = typename getter_traits<G>::field_type;
                this->template for_each_column_with_field_type<field_type>([&res, getter](auto& c) {
                    if(compare_any(c.getter, getter)) {
                        res = &c.name;
                    }
                });
                return res;
            }

            /**
             *  Searches column name by class setter function member pointer passed as first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class S>
            const std::string* find_column_name(S setter,
                                                typename std::enable_if<is_setter<S>::value>::type* = nullptr) const {
                const std::string* res = nullptr;
                using field_type = typename setter_traits<S>::field_type;
                this->template for_each_column_with_field_type<field_type>([&res, setter](auto& c) {
                    if(compare_any(c.setter, setter)) {
                        res = &c.name;
                    }
                });
                return res;
            }

            /**
             *  Counts and returns amount of columns without GENERATED ALWAYS constraints. Skips table constraints.
             */
            int non_generated_columns_count() const {
                auto res = 0;
                this->for_each_column([&res](auto& column) {
                    if(!column.is_generated()) {
                        ++res;
                    }
                });
                return res;
            }

            /**
             *  Counts and returns amount of columns. Skips constraints.
             */
            int count_columns_amount() const {
                auto res = 0;
                this->for_each_column([&res](auto&) {
                    ++res;
                });
                return res;
            }

            /**
             *  Iterates all columns and fires passed lambda. Lambda must have one and only templated argument. Otherwise
             *  code will not compile. Excludes table constraints (e.g. foreign_key_t) at the end of the columns list. To
             *  iterate columns with table constraints use iterate_tuple(columns, ...) instead. L is lambda type. Do
             *  not specify it explicitly.
             *  @param lambda Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class L>
            void for_each_column(const L& lambda) const {
                iterate_tuple(this->elements, [&lambda](auto& element) {
                    using element_type = typename std::decay<decltype(element)>::type;
                    static_if<is_column<element_type>{}>(lambda)(element);
                });
            }

            template<class L>
            void for_each_foreign_key(const L& lambda) const {
                iterate_tuple(this->elements, [&lambda](auto& element) {
                    using element_type = typename std::decay<decltype(element)>::type;
                    static_if<is_foreign_key<element_type>{}>(lambda)(element);
                });
            }

            template<class F, class L>
            void for_each_column_with_field_type(const L& lambda) const {
                this->for_each_column([&lambda](auto& column) {
                    using column_type = typename std::decay<decltype(column)>::type;
                    using field_type = typename column_field_type<column_type>::type;
                    static_if<std::is_same<F, field_type>{}>(lambda)(column);
                });
            }

            /**
             *  Iterates all columns that have specified constraints and fires passed lambda.
             *  Lambda must have one and only templated argument Otherwise code will not compile.
             *  L is lambda type. Do not specify it explicitly.
             *  @param lambda Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class Op, class L>
            void for_each_column_with(const L& lambda) const {
                this->for_each_column([&lambda](auto& column) {
                    using tuple_helper::tuple_contains_type;
                    using column_type = typename std::decay<decltype(column)>::type;
                    using constraints_type = typename column_constraints_type<column_type>::type;
                    static_if<tuple_contains_type<Op, constraints_type>{}>(lambda)(column);
                });
            }

            std::vector<table_info> get_table_info() const {
                std::vector<table_info> res;
                res.reserve(size_t(this->elements_count));
                this->for_each_column([&res](auto& col) {
                    std::string dft;
                    using field_type = typename std::decay<decltype(col)>::type::field_type;
                    if(auto d = col.default_value()) {
                        dft = *d;
                    }
                    table_info i{
                        -1,
                        col.name,
                        type_printer<field_type>().print(),
                        col.not_null(),
                        dft,
                        col.template has<primary_key_t<>>(),
                    };
                    res.emplace_back(i);
                });
                auto compositeKeyColumnNames = this->composite_key_columns_names();
                for(size_t i = 0; i < compositeKeyColumnNames.size(); ++i) {
                    auto& columnName = compositeKeyColumnNames[i];
                    auto it = std::find_if(res.begin(), res.end(), [&columnName](const table_info& ti) {
                        return ti.name == columnName;
                    });
                    if(it != res.end()) {
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
    template<class... Cs, class T = typename std::tuple_element<0, std::tuple<Cs...>>::type::object_type>
    internal::table_t<T, false, Cs...> make_table(const std::string& name, Cs... args) {
        return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)};
    }

    template<class T, class... Cs>
    internal::table_t<T, false, Cs...> make_table(const std::string& name, Cs... args) {
        return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)};
    }
}
