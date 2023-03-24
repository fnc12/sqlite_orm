#pragma once

#include <string>  //  std::string
#include <type_traits>  //  std::remove_reference, std::is_same, std::decay
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_size, std::tuple_element
#include <utility>  //  std::forward, std::move

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "functional/cxx_functional_polyfill.h"
#include "functional/static_magic.h"
#include "functional/mpl.h"
#include "functional/index_sequence_util.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_iteration.h"
#include "member_traits/member_traits.h"
#include "typed_comparator.h"
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
        template<class O, bool WithoutRowId, class... Cs>
        struct table_t : basic_table {
            using object_type = O;
            using elements_type = std::tuple<Cs...>;

            static constexpr bool is_without_rowid_v = WithoutRowId;
            using is_without_rowid = polyfill::bool_constant<is_without_rowid_v>;

            elements_type elements;

#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            table_t(std::string name_, elements_type elements_) :
                basic_table{std::move(name_)}, elements{std::move(elements_)} {}
#endif

            table_t<O, true, Cs...> without_rowid() const {
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
                using field_type = member_field_type_t<M>;
                const field_type* res = nullptr;
                iterate_tuple(this->elements,
                              col_index_sequence_with_field_type<elements_type, field_type>{},
                              call_as_template_base<column_field>([&res, &memberPointer, &object](const auto& column) {
                                  if(compare_any(column.setter, memberPointer)) {
                                      res = &polyfill::invoke(column.member_pointer, object);
                                  }
                              }));
                return res;
            }

            const basic_generated_always::storage_type*
            find_column_generated_storage_type(const std::string& name) const {
                const basic_generated_always::storage_type* result = nullptr;
#if SQLITE_VERSION_NUMBER >= 3031000
                iterate_tuple(this->elements,
                              col_index_sequence_with<elements_type, is_generated_always>{},
                              [&result, &name](auto& column) {
                                  if(column.name != name) {
                                      return;
                                  }
                                  using generated_op_index_sequence =
                                      filter_tuple_sequence_t<std::remove_const_t<decltype(column.constraints)>,
                                                              is_generated_always>;
                                  constexpr size_t opIndex = first_index_sequence_value(generated_op_index_sequence{});
                                  result = &get<opIndex>(column.constraints).storage;
                              });
#else
                (void)name;
#endif
                return result;
            }

            template<class G, class S>
            bool exists_in_composite_primary_key(const column_field<G, S>& column) const {
                bool res = false;
                this->for_each_primary_key([&column, &res](auto& primaryKey) {
                    using colrefs_tuple = decltype(primaryKey.columns);
                    using same_type_index_sequence =
                        filter_tuple_sequence_t<colrefs_tuple,
                                                check_if_is_type<member_field_type_t<G>>::template fn,
                                                member_field_type_t>;
                    iterate_tuple(primaryKey.columns, same_type_index_sequence{}, [&res, &column](auto& memberPointer) {
                        if(compare_any(memberPointer, column.member_pointer) ||
                           compare_any(memberPointer, column.setter)) {
                            res = true;
                        }
                    });
                });
                return res;
            }

            /**
             *  Call passed lambda with all defined primary keys.
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
                using pkcol_index_sequence = col_index_sequence_with<elements_type, is_primary_key>;

                if(pkcol_index_sequence::size() > 0) {
                    return create_from_tuple<std::vector<std::string>>(this->elements,
                                                                       pkcol_index_sequence{},
                                                                       &column_identifier::name);
                } else {
                    return this->composite_key_columns_names();
                }
            }

            template<class L>
            void for_each_primary_key_column(L&& lambda) const {
                iterate_tuple(this->elements,
                              col_index_sequence_with<elements_type, is_primary_key>{},
                              call_as_template_base<column_field>([&lambda](const auto& column) {
                                  lambda(column.member_pointer);
                              }));
                this->for_each_primary_key([&lambda](auto& primaryKey) {
                    iterate_tuple(primaryKey.columns, lambda);
                });
            }

            template<class... Args>
            std::vector<std::string> composite_key_columns_names(const primary_key_t<Args...>& primaryKey) const {
                return create_from_tuple<std::vector<std::string>>(primaryKey.columns,
                                                                   [this, empty = std::string{}](auto& memberPointer) {
                                                                       if(const std::string* columnName =
                                                                              this->find_column_name(memberPointer)) {
                                                                           return *columnName;
                                                                       } else {
                                                                           return empty;
                                                                       }
                                                                   });
            }

            /**
             *  Searches column name by class member pointer passed as the first argument.
             *  @return column name or empty string if nothing found.
             */
            template<class M, satisfies<std::is_member_pointer, M> = true>
            const std::string* find_column_name(M m) const {
                const std::string* res = nullptr;
                using field_type = member_field_type_t<M>;
                iterate_tuple(this->elements,
                              col_index_sequence_with_field_type<elements_type, field_type>{},
                              [&res, m](auto& c) {
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
                using non_generated_col_index_sequence =
                    col_index_sequence_excluding<elements_type, is_generated_always>;
                return int(non_generated_col_index_sequence::size());
#else
                return this->count_columns_amount();
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
             *  @param lambda Lambda called for each column. Function signature: `void(auto& column)`
             */
            template<class L>
            void for_each_foreign_key(L&& lambda) const {
                using fk_index_sequence = filter_tuple_sequence_t<elements_type, is_foreign_key>;
                iterate_tuple(this->elements, fk_index_sequence{}, lambda);
            }

            template<class Target, class L>
            void for_each_foreign_key_to(L&& lambda) const {
                using fk_index_sequence = filter_tuple_sequence_t<elements_type, is_foreign_key>;
                using filtered_index_sequence = filter_tuple_sequence_t<elements_type,
                                                                        check_if_is_type<Target>::template fn,
                                                                        target_type_t,
                                                                        fk_index_sequence>;
                iterate_tuple(this->elements, filtered_index_sequence{}, lambda);
            }

            /**
             *  Call passed lambda with all defined columns.
             *  @param lambda Lambda called for each column. Function signature: `void(auto& column)`
             */
            template<class L>
            void for_each_column(L&& lambda) const {
                using col_index_sequence = filter_tuple_sequence_t<elements_type, is_column>;
                iterate_tuple(this->elements, col_index_sequence{}, lambda);
            }

            /**
             *  Call passed lambda with columns not having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda called for each column.
             */
            template<template<class...> class OpTraitFn, class L>
            void for_each_column_excluding(L&& lambda) const {
                iterate_tuple(this->elements, col_index_sequence_excluding<elements_type, OpTraitFn>{}, lambda);
            }

            /**
             *  Call passed lambda with columns not having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda called for each column.
             */
            template<class OpTraitFnCls, class L, satisfies<mpl::is_metafunction_class, OpTraitFnCls> = true>
            void for_each_column_excluding(L&& lambda) const {
                this->for_each_column_excluding<OpTraitFnCls::template fn>(lambda);
            }

            std::vector<table_xinfo> get_table_info() const;
        };

        template<class T>
        struct is_table : std::false_type {};

        template<class O, bool W, class... Cs>
        struct is_table<table_t<O, W, Cs...>> : std::true_type {};
    }

    /**
     *  Factory function for a table definition.
     *
     *  The mapped object type is determined implicitly from the first column definition.
     */
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::table_t<T, false, Cs...> make_table(std::string name, Cs... args) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), std::make_tuple<Cs...>(std::forward<Cs>(args)...)});
    }

    /**
     *  Factory function for a table definition.
     * 
     *  The mapped object type is explicitly specified.
     */
    template<class T, class... Cs>
    internal::table_t<T, false, Cs...> make_table(std::string name, Cs... args) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), std::make_tuple<Cs...>(std::forward<Cs>(args)...)});
    }
}
