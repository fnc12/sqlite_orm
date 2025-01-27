#pragma once

#include <string>  //  std::string
#include <type_traits>  //  std::remove_const, std::is_member_pointer, std::true_type, std::false_type
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_element
#include <utility>  //  std::forward, std::move

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cxx_functional_polyfill.h"
#include "../functional/static_magic.h"
#include "../functional/mpl.h"
#include "../functional/index_sequence_util.h"
#include "../tuple_helper/tuple_filter.h"
#include "../tuple_helper/tuple_traits.h"
#include "../tuple_helper/tuple_iteration.h"
#include "../tuple_helper/tuple_transformer.h"
#include "../member_traits/member_traits.h"
#include "../typed_comparator.h"
#include "../type_traits.h"
#include "../alias_traits.h"
#include "../constraints.h"
#include "../table_info.h"
#include "index.h"
#include "column.h"

namespace sqlite_orm {

    namespace internal {

        template<class T>
        using is_table_element_or_constraint = mpl::invoke_t<mpl::disjunction<check_if<is_column>,
                                                                              check_if<is_primary_key>,
                                                                              check_if<is_foreign_key>,
                                                                              check_if_is_template<index_t>,
                                                                              check_if_is_template<unique_t>,
                                                                              check_if_is_template<check_t>,
                                                                              check_if_is_template<prefix_t>,
                                                                              check_if_is_template<tokenize_t>,
                                                                              check_if_is_template<content_t>,
                                                                              check_if_is_template<table_content_t>>,
                                                             T>;

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
        /**
         *  A subselect mapper's CTE moniker, void otherwise.
         */
        template<typename O>
        using moniker_of_or_void_t = polyfill::detected_or_t<void, cte_moniker_type_t, O>;

        /** 
         *  If O is a subselect_mapper then returns its nested type name O::cte_moniker_type,
         *  otherwise O itself is a regular object type to be mapped.
         */
        template<typename O>
        using mapped_object_type_for_t = polyfill::detected_or_t<O, cte_moniker_type_t, O>;
#endif

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
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            // this typename is used in contexts where it is known that the 'table' holds a subselect_mapper
            // instead of a regular object type
            using cte_mapper_type = O;
            using cte_moniker_type = moniker_of_or_void_t<O>;
            using object_type = mapped_object_type_for_t<O>;
#else
            using object_type = O;
#endif
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

            /*
             *  Returns the number of elements of the specified type.
             */
            template<template<class...> class Trait>
            static constexpr int count_of() {
                using sequence_of = filter_tuple_sequence_t<elements_type, Trait>;
                return int(sequence_of::size());
            }

            /*
             *  Returns the number of columns having the specified constraint trait.
             */
            template<template<class...> class Trait>
            static constexpr int count_of_columns_with() {
                using filtered_index_sequence = col_index_sequence_with<elements_type, Trait>;
                return int(filtered_index_sequence::size());
            }

            /*
             *  Returns the number of columns having the specified constraint trait.
             */
            template<template<class...> class Trait>
            static constexpr int count_of_columns_excluding() {
                using excluded_col_index_sequence = col_index_sequence_excluding<elements_type, Trait>;
                return int(excluded_col_index_sequence::size());
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
                                  if (compare_any(column.setter, memberPointer)) {
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
                                  if (column.name != name) {
                                      return;
                                  }
                                  using generated_op_index_sequence =
                                      filter_tuple_sequence_t<std::remove_const_t<decltype(column.constraints)>,
                                                              is_generated_always>;
                                  constexpr size_t opIndex = index_sequence_value_at<0>(generated_op_index_sequence{});
                                  result = &std::get<opIndex>(column.constraints).storage;
                              });
#else
                (void)name;
#endif
                return result;
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

                if (pkcol_index_sequence::size() > 0) {
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
                                                                       if (const std::string* columnName =
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
                                  if (compare_any(c.member_pointer, m) || compare_any(c.setter, m)) {
                                      res = &c.name;
                                  }
                              });
                return res;
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
            template<class OpTraitQ, class L, satisfies<mpl::is_quoted_metafuntion, OpTraitQ> = true>
            void for_each_column_excluding(L&& lambda) const {
                this->template for_each_column_excluding<OpTraitQ::template fn>(lambda);
            }

            std::vector<table_xinfo> get_table_info() const;
        };

        template<class T>
        struct is_table : std::false_type {};

        template<class O, bool W, class... Cs>
        struct is_table<table_t<O, W, Cs...>> : std::true_type {};

        template<class M>
        struct virtual_table_t : basic_table {
            using module_details_type = M;
            using object_type = typename module_details_type::object_type;
            using elements_type = typename module_details_type::columns_type;

            static constexpr bool is_without_rowid_v = false;
            using is_without_rowid = polyfill::bool_constant<is_without_rowid_v>;

            module_details_type module_details;

#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            virtual_table_t(std::string name, module_details_type module_details) :
                basic_table{std::move(name)}, module_details{std::move(module_details)} {}
#endif

            /**
             *  Call passed lambda with columns not having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda called for each column.
             */
            template<template<class...> class OpTraitFn, class L>
            void for_each_column_excluding(L&& lambda) const {
                this->module_details.template for_each_column_excluding<OpTraitFn>(lambda);
            }

            /**
             *  Call passed lambda with columns not having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda called for each column.
             */
            template<class OpTraitQ, class L, satisfies<mpl::is_quoted_metafuntion, OpTraitQ> = true>
            void for_each_column_excluding(L&& lambda) const {
                this->module_details.template for_each_column_excluding<OpTraitQ>(lambda);
            }

            /**
             *  Call passed lambda with all defined columns.
             *  @param lambda Lambda called for each column. Function signature: `void(auto& column)`
             */
            template<class L>
            void for_each_column(L&& lambda) const {
                this->module_details.for_each_column(lambda);
            }
        };

        template<class T>
        struct is_virtual_table : std::false_type {};

        template<class M>
        struct is_virtual_table<virtual_table_t<M>> : std::true_type {};

#if SQLITE_VERSION_NUMBER >= 3009000
        template<class T, class... Cs>
        struct using_fts5_t {
            using object_type = T;
            using columns_type = std::tuple<Cs...>;

            columns_type columns;

            using_fts5_t(columns_type columns) : columns(std::move(columns)) {}

            /**
             *  Call passed lambda with columns not having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda called for each column.
             */
            template<template<class...> class OpTraitFn, class L>
            void for_each_column_excluding(L&& lambda) const {
                iterate_tuple(this->columns, col_index_sequence_excluding<columns_type, OpTraitFn>{}, lambda);
            }

            /**
             *  Call passed lambda with columns not having the specified constraint trait `OpTrait`.
             *  @param lambda Lambda called for each column.
             */
            template<class OpTraitQ, class L, satisfies<mpl::is_quoted_metafuntion, OpTraitQ> = true>
            void for_each_column_excluding(L&& lambda) const {
                this->template for_each_column_excluding<OpTraitQ::template fn>(lambda);
            }

            /**
             *  Call passed lambda with all defined columns.
             *  @param lambda Lambda called for each column. Function signature: `void(auto& column)`
             */
            template<class L>
            void for_each_column(L&& lambda) const {
                using col_index_sequence = filter_tuple_sequence_t<columns_type, is_column>;
                iterate_tuple(this->columns, col_index_sequence{}, lambda);
            }
        };
#endif

        template<class O, bool WithoutRowId, class... Cs, class G, class S>
        bool exists_in_composite_primary_key(const table_t<O, WithoutRowId, Cs...>& table,
                                             const column_field<G, S>& column) {
            bool res = false;
            table.for_each_primary_key([&column, &res](auto& primaryKey) {
                using colrefs_tuple = decltype(primaryKey.columns);
                using same_type_index_sequence =
                    filter_tuple_sequence_t<colrefs_tuple,
                                            check_if_is_type<member_field_type_t<G>>::template fn,
                                            member_field_type_t>;
                iterate_tuple(primaryKey.columns, same_type_index_sequence{}, [&res, &column](auto& memberPointer) {
                    if (compare_any(memberPointer, column.member_pointer) ||
                        compare_any(memberPointer, column.setter)) {
                        res = true;
                    }
                });
            });
            return res;
        }

        template<class M, class G, class S>
        bool exists_in_composite_primary_key(const virtual_table_t<M>& /*virtualTable*/,
                                             const column_field<G, S>& /*column*/) {
            return false;
        }
    }

#if SQLITE_VERSION_NUMBER >= 3009000
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::using_fts5_t<T, Cs...> using_fts5(Cs... columns) {
        static_assert(polyfill::conjunction_v<internal::is_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(columns)...)});
    }

    template<class T, class... Cs>
    internal::using_fts5_t<T, Cs...> using_fts5(Cs... columns) {
        static_assert(polyfill::conjunction_v<internal::is_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(columns)...)});
    }
#endif

    /**
     *  Factory function for a table definition.
     *  
     *  The mapped object type is determined implicitly from the first column definition.
     */
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::table_t<T, false, Cs...> make_table(std::string name, Cs... args) {
        static_assert(polyfill::conjunction_v<internal::is_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

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
        static_assert(polyfill::conjunction_v<internal::is_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), std::make_tuple<Cs...>(std::forward<Cs>(args)...)});
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Factory function for a table definition.
     *  
     *  The mapped object type is explicitly specified.
     */
    template<orm_table_reference auto table, class... Cs>
    auto make_table(std::string name, Cs... args) {
        return make_table<internal::auto_decay_table_ref_t<table>>(std::move(name), std::forward<Cs>(args)...);
    }
#endif

    template<class M>
    internal::virtual_table_t<M> make_virtual_table(std::string name, M module_details) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::move(name), std::move(module_details)});
    }
}
