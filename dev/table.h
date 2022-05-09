#pragma once

#include <string>  //  std::string
#include <type_traits>  //  std::remove_reference, std::is_same, std::decay
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_size, std::tuple_element

#include "cxx_functional_polyfill.h"
#include "static_magic.h"
#include "typed_comparator.h"
#include "tuple_helper/tuple_helper.h"
#include "tuple_helper/tuple_filter.h"
#include "member_traits/member_traits.h"
#include "type_traits.h"
#include "constraints.h"
#include "table_info.h"
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
            using object_type = T;
            using elements_type = std::tuple<Cs...>;

            static constexpr int elements_count = static_cast<int>(std::tuple_size<elements_type>::value);
            static constexpr bool is_without_rowid = WithoutRowId;

            elements_type elements;

#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            table_t(std::string name_, elements_type elements_) : basic_table{move(name_)}, elements{move(elements_)} {}
#endif

            table_t<T, true, Cs...> without_rowid() const {
                return {this->name, this->elements};
            }

            /**
             *  Returns foreign keys count in table definition
             */
            constexpr int foreign_keys_count() const {
#if SQLITE_VERSION_NUMBER >= 3006019
                return int(filter_tuple_sequence_t<elements_type, is_foreign_key>::size());
#else
                return 0;
#endif
            }

            /**
             *  Function used to get field value from object by mapped member pointer/setter/getter
             */
            template<class F, class C>
            const F* get_object_field_pointer(const object_type& object, C memberPointer) const {
                const F* res = nullptr;
                this->for_each_column_with_field_type<F>([&res, &memberPointer, &object](auto& column) {
                    if(res) {
                        return;
                    }

                    if(compare_any(column.member_pointer, memberPointer) || compare_any(column.setter, memberPointer)) {
                        res = &polyfill::invoke(column.member_pointer, object);
                    }
                });
                return res;
            }

            const basic_generated_always::storage_type*
            find_column_generated_storage_type(const std::string& name) const {
                const basic_generated_always::storage_type* result = nullptr;
#if SQLITE_VERSION_NUMBER >= 3031000
                this->for_each_column([&result, &name](auto& column) {
                    if(column.name != name) {
                        return;
                    }
                    iterate_tuple(column.constraints, [&result](auto& constraint) {
                        if(result) {
                            return;
                        }
                        using constraint_type = std::decay_t<decltype(constraint)>;
                        static_if<is_generated_always_v<constraint_type>>([&result](auto& generatedAlwaysConstraint) {
                            result = &generatedAlwaysConstraint.storage;
                        })(constraint);
                    });
                });
#endif
                return result;
            }

            template<class C>
            bool exists_in_composite_primary_key(const C& column) const {
                auto res = false;
                this->for_each_primary_key([&column, &res](auto& primaryKey) {
                    iterate_tuple(primaryKey.columns, [&res, &column](auto& value) {
                        if(res) {
                            return;
                        }

                        res = compare_any(value, column.member_pointer) || compare_any(value, column.setter);
                    });
                });
                return res;
            }

            /**
             *  Calls **l** with every primary key dedicated constraint
             */
            template<class L>
            void for_each_primary_key(L&& lambda) const {
                iterate_tuple(this->elements, [&lambda](auto& element) {
                    using element_type = std::decay_t<decltype(element)>;
                    static_if<is_primary_key_v<element_type>>(lambda)(element);
                });
            }

            std::vector<std::string> composite_key_columns_names() const {
                std::vector<std::string> res;
                this->for_each_primary_key([this, &res](auto& primaryKey) {
                    res = this->composite_key_columns_names(primaryKey);
                });
                return res;
            }

            std::vector<std::string> primary_key_column_names() const {
                std::vector<std::string> res;
                this->for_each_column_with<primary_key_t<>>([&res](auto& column) {
                    res.push_back(column.name);
                });
                if(res.empty()) {
                    res = this->composite_key_columns_names();
                }
                return res;
            }

            template<class L>
            void for_each_primary_key_column(L&& lambda) const {
                this->for_each_column_with<primary_key_t<>>([&lambda](auto& column) {
                    lambda(column.member_pointer);
                });
                this->for_each_primary_key([this, &lambda](auto& primaryKey) {
                    this->for_each_column_in_primary_key(primaryKey, lambda);
                });
            }

            template<class L, class... Args>
            void for_each_column_in_primary_key(const primary_key_t<Args...>& primarykey, L&& lambda) const {
                iterate_tuple(primarykey.columns, lambda);
            }

            template<class... Args>
            std::vector<std::string> composite_key_columns_names(const primary_key_t<Args...>& pk) const {
                std::vector<std::string> res;
                using pk_columns_tuple = decltype(pk.columns);
                res.reserve(std::tuple_size<pk_columns_tuple>::value);
                iterate_tuple(pk.columns, [this, &res](auto& memberPointer) {
                    if(auto* columnName = this->find_column_name(memberPointer)) {
                        res.push_back(*columnName);
                    } else {
                        res.emplace_back();
                    }
                });
                return res;
            }

            /**
             *  Searches column name by class member pointer passed as the first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class M, satisfies<std::is_member_pointer, M> = true>
            const std::string* find_column_name(M m) const {
                const std::string* res = nullptr;
                using field_type = member_field_type_t<M>;
                this->template for_each_column_with_field_type<field_type>([&res, m](auto& c) {
                    if(compare_any(c.member_pointer, m) || compare_any(c.setter, m)) {
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
            constexpr int count_columns_amount() const {
                return int(filter_tuple_sequence_t<elements_type, is_column>::size());
            }

            /**
             *  Iterates all columns and fires passed lambda. Lambda must have one and only templated argument. Otherwise
             *  code will not compile. Excludes table constraints (e.g. foreign_key_t) at the end of the columns list. To
             *  iterate columns with table constraints use iterate_tuple(columns, ...) instead. L is lambda type. Do
             *  not specify it explicitly.
             *  @param lambda Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class L>
            void for_each_column(L&& lambda) const {
                iterate_tuple(this->elements, [&lambda](auto& element) {
                    using element_type = std::decay_t<decltype(element)>;
                    static_if<is_column_v<element_type>>(lambda)(element);
                });
            }

            template<class L>
            void for_each_foreign_key(L&& lambda) const {
                iterate_tuple(this->elements, [&lambda](auto& element) {
                    using element_type = std::decay_t<decltype(element)>;
                    static_if<is_foreign_key_v<element_type>>(lambda)(element);
                });
            }

            template<class F, class L>
            void for_each_column_with_field_type(L&& lambda) const {
                this->for_each_column([&lambda](auto& column) {
                    using column_type = std::decay_t<decltype(column)>;
                    using field_type = typename column_field_type<column_type>::type;
                    static_if<std::is_same<F, field_type>::value>(lambda)(column);
                });
            }

            /**
             *  Iterates all columns that have specified constraints and fires passed lambda.
             *  Lambda must have one and only templated argument Otherwise code will not compile.
             *  L is lambda type. Do not specify it explicitly.
             *  @param lambda Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
             */
            template<class Op, class L>
            void for_each_column_with(L&& lambda) const {
                this->for_each_column([&lambda](auto& column) {
                    using tuple_helper::tuple_contains_type;
                    using column_type = std::decay_t<decltype(column)>;
                    using constraints_type = typename column_constraints_type<column_type>::type;
                    constexpr bool c = tuple_contains_type<Op, constraints_type>::value;
                    static_if<c>(lambda)(column);
                });
            }

            std::vector<table_xinfo> get_table_info() const;
        };
    }

    /**
     *  Function used for table creation. Do not use table constructor - use this function
     *  cause table class is templated and its constructing too (just like std::make_unique or std::make_pair).
     */
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::table_t<T, false, Cs...> make_table(const std::string& name, Cs... args) {
        return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)};
    }

    template<class T, class... Cs>
    internal::table_t<T, false, Cs...> make_table(const std::string& name, Cs... args) {
        return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)};
    }
}
