#pragma once

#include <string>  //  std::string
#include <type_traits>  //  std::remove_reference, std::is_same, std::decay
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_size, std::tuple_element
#include <utility>  //  std::forward, std::move

#include "functional/cxx_universal.h"
#include "functional/cxx_polyfill.h"
#include "functional/cxx_functional_polyfill.h"
#include "functional/static_magic.h"
#include "functional/mpl.h"
#include "typed_comparator.h"
#include "tuple_helper/tuple_iteration.h"
#include "tuple_helper/tuple_traits.h"
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
         *  Table definition.
         */
        template<class T, bool WithoutRowId, class... Cs>
        struct table_t : basic_table {
            using object_type = T;
            using elements_type = std::tuple<Cs...>;

            static constexpr bool is_without_rowid_v = WithoutRowId;
            using is_without_rowid = polyfill::bool_constant<is_without_rowid_v>;
            static constexpr int elements_count = static_cast<int>(std::tuple_size<elements_type>::value);

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
                using fk_index_sequence = filter_tuple_sequence_t<elements_type, is_foreign_key>;
                return int(fk_index_sequence::size());
#else
                return 0;
#endif
            }

            /**
             *  Function used to get field value from object by mapped member pointer/setter/getter.
             *  
             *  For a setter the corresponding getter has to be searched,
             *  so the method returns a pointer to the field as returned by the found getter.
             *  Otherwise the method invokes the member pointer and returns its result.
             */
            template<class M, satisfies_not<is_setter, M> = true>
            decltype(auto) object_field_value(const object_type& object, M memberPointer) const {
                return polyfill::invoke(memberPointer, object);
            }

            template<class M, satisfies<is_setter, M> = true>
            const member_field_type_t<M>* object_field_value(const object_type& object, M memberPointer) const {
                using F = member_field_type_t<M>;
                const F* res = nullptr;
                this->for_each_column_with_field_type<F>([&res, &memberPointer, &object](auto& column) {
                    if(res) {
                        return;
                    }

                    if(compare_any(column.setter, memberPointer)) {
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

                    using generated_col_seq =
                        filter_tuple_sequence_t<polyfill::remove_cvref_t<decltype(column.constraints)>,
                                                is_generated_always>;
                    iterate_tuple(column.constraints, generated_col_seq{}, [&result](auto& constraint) {
                        if(!result) {
                            result = &constraint.storage;
                        }
                    });
                });
#endif
                return result;
            }

            template<class C>
            bool exists_in_composite_primary_key(const C& column) const {
                auto res = false;
                this->for_each_primary_key([&column, &res](auto& primaryKey) {
                    iterate_tuple(primaryKey.columns, [&res, &column](auto& memberPointer) {
                        if(!res) {
                            res = compare_any(memberPointer, column.member_pointer) ||
                                  compare_any(memberPointer, column.setter);
                        }
                    });
                });
                return res;
            }

            /**
             *  Calls **l** with every primary key dedicated constraint
             */
            template<class L>
            void for_each_primary_key(L&& lambda) const {
                using pk_index_sequence = filter_tuple_sequence_t<elements_type, is_primary_key>;
                iterate_tuple(this->elements, pk_index_sequence{}, lambda);
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
                this->for_each_column_with<is_primary_key>([&res](auto& column) {
                    res.push_back(column.name);
                });
                if(res.empty()) {
                    res = this->composite_key_columns_names();
                }
                return res;
            }

            template<class L>
            void for_each_primary_key_column(L&& lambda) const {
                this->for_each_column_with<is_primary_key>([&lambda](auto& column) {
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
            constexpr int non_generated_columns_count() const {
#if SQLITE_VERSION_NUMBER >= 3031000
                //using excluding_trait_fn = ...;
                using col_seq = filter_tuple_sequence_t<elements_type, is_column>;
                using non_generated_col_seq =
                    filter_tuple_sequence_t<elements_type,
                                            mpl::not_<check_if_tuple_has<is_generated_always>>::template fn,
                                            column_constraints_type_t,
                                            col_seq>;
                return int(non_generated_col_seq::size());
#else
                return 0;
#endif
            }

            /**
             *  Counts and returns amount of columns. Skips constraints.
             */
            constexpr int count_columns_amount() const {
                using col_index_sequence = filter_tuple_sequence_t<elements_type, is_column>;
                return int(col_index_sequence::size());
            }

            /**
             *  Call passed lambda with all defined foreign keys.
             *  @param lambda Lambda to be called for each column. Must have signature like this [] (auto col) -> void {}
             */
            template<class L>
            void for_each_foreign_key(L&& lambda) const {
                using fk_index_sequence = filter_tuple_sequence_t<elements_type, is_foreign_key>;
                iterate_tuple(this->elements, fk_index_sequence{}, lambda);
            }

            /**
             *  Call passed lambda with all defined columns.
             *  @param lambda Lambda to be called for each column. Must have signature like this [] (auto col) -> void {}
             */
            template<class L>
            void for_each_column(L&& lambda) const {
                using col_index_sequence = filter_tuple_sequence_t<elements_type, is_column>;
                iterate_tuple(this->elements, col_index_sequence{}, lambda);
            }

            /**
             *  Call passed lambda with columns filtered on `Predicate(Transform(column_t))`.
             *  @param lambda Lambda to be called for each column. Must have signature like this [] (auto col) -> void {}
             */
            template<template<class...> class Transform, template<class...> class PredicateFn, class L>
            void for_each_column(L&& lambda) const {
                using col_seq = filter_tuple_sequence_t<elements_type, is_column>;
                using idx_seq = filter_tuple_sequence_t<elements_type, PredicateFn, Transform, col_seq>;
                iterate_tuple(this->elements, idx_seq{}, lambda);
            }

            /**
             *  Call passed lambda with columns filtered on `Predicate(Transform(column_t))`.
             *  @param lambda Lambda to be called for each column. Must have signature like this [] (auto col) -> void {}
             */
            template<template<class...> class Transform,
                     class PredicateFnCls,
                     class L,
                     satisfies<mpl::is_metafunction_class, PredicateFnCls> = true>
            void for_each_column(L&& lambda) const {
                using col_seq = filter_tuple_sequence_t<elements_type, is_column>;
                using idx_seq =
                    filter_tuple_sequence_t<elements_type, typename PredicateFnCls::template fn, Transform, col_seq>;
                iterate_tuple(this->elements, idx_seq{}, lambda);
            }

            /**
             *  Call passed lambda with columns having the specified field type `F`.
             *  @param lambda Lambda to be called for each column. Must have signature like this [] (auto col) -> void {}
             */
            template<class F, class L>
            void for_each_column_with_field_type(L&& lambda) const {
                //using is_field_type_fn = ...;
                this->for_each_column<column_field_type_t, mpl::bind_front_fn<std::is_same, F>>(lambda);
            }

            /**
             *  Call passed lambda with columns having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda to be called for each column. Must have signature like this [] (auto col) -> void {}
             */
            template<template<class...> class OpTraitFn, class L>
            void for_each_column_with(L&& lambda) const {
                this->for_each_column<column_constraints_type_t, check_if_tuple_has<OpTraitFn>>(lambda);
            }

            /**
             *  Call passed lambda with columns not having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda to be called for each column. Must have signature like this [] (auto col) -> void {}
             */
            template<template<class...> class OpTraitFn, class L>
            void for_each_column_excluding(L&& lambda) const {
                this->for_each_column<column_constraints_type_t, mpl::not_<check_if_tuple_has<OpTraitFn>>>(lambda);
            }
            template<class OpTraitFnCls, class L, satisfies<mpl::is_metafunction_class, OpTraitFnCls> = true>
            void for_each_column_excluding(L&& lambda) const {
                this->for_each_column<column_constraints_type_t,
                                      mpl::not_<check_if_tuple_has<mpl::fn_t<OpTraitFnCls>>>>(lambda);
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
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)});
    }

    template<class T, class... Cs>
    internal::table_t<T, false, Cs...> make_table(const std::string& name, Cs... args) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {name, std::make_tuple<Cs...>(std::forward<Cs>(args)...)});
    }
}
