#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_member_pointer, std::remove_cvref
#include <string>  //  std::string
#include <tuple>  // std::tuple, std::tuple_size
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cxx_functional_polyfill.h"
#include "../functional/mpl.h"
#include "../tuple_helper/tuple_filter.h"
#include "../tuple_helper/tuple_iteration.h"
#include "../tuple_helper/tuple_transformer.h"
#include "../member_traits/member_traits.h"
#include "../type_traits.h"
#include "../field_of.h"
#include "column.h"

namespace sqlite_orm::internal {

    struct table_identifier {

        /**
         *  Table name.
         */
        std::string name;
    };

    /** 
     *  Encapsulates table elements, i.e. columns and constraints for any type of table.
     */
    template<class... Cs>
    struct table_definition {
        using elements_type = std::tuple<Cs...>;

        elements_type elements;

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
            using col_index_sequence = col_index_sequence_with<elements_type, Trait>;
            return int(col_index_sequence::size());
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
         *  Call passed lambda with all defined columns.
         *  @param lambda Lambda called for each column. Function signature: `void(auto& column)`
         */
        template<class L>
        void for_each_column(L&& lambda) const {
            iterate_tuple(this->elements, col_index_sequence_of<elements_type>{}, lambda);
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

        /**
         *  Finds the column name by the given class member pointer.
         *  @return column name or nullptr if nothing found.
         */
        template<class M, satisfies<std::is_member_pointer, M> = true>
        const std::string* find_column_name(M memberPointer) const {
            using field_type = member_field_type_t<M>;

            const std::string* res = nullptr;
            iterate_tuple(this->elements,
                          all_col_index_sequence_with_field_type<elements_type, field_type>{},
                          [&res, memberPointer](auto& column) {
                              if (compare_fields(column.member_pointer, memberPointer) ||
                                  compare_fields(column.setter, memberPointer)) {
                                  res = &column.name;
                              }
                          });
            return res;
        }
    };

    /** 
     *  Encapsulates table elements, i.e. columns and constraints for a type of table that can have a primary key - base tables and usually virtual tables -,
     *  and provides additional methods to those of a generic table definition in order to deal with primary key columns.
     */
    template<class... Cs>
    struct insertable_table_definition : table_definition<Cs...> {
        using definition_base_type = table_definition<Cs...>;
        using elements_type = elements_type_t<definition_base_type>;

        /**
         *  Call passed lambda with the defined table primary key.
         */
        template<class L>
        void visit_table_primary_key(L&& lambda) const {
            using pk_index_sequence = filter_tuple_sequence_t<elements_type, is_primary_key>;
            // note: already checked in `validate_base_table_definition()`
            static_assert(pk_index_sequence::size() <= 1);
            // note: we use the tuple iteration function for simplicity, even if we know there is at most one primary key
            iterate_tuple(this->elements, pk_index_sequence{}, lambda);
        }

        std::vector<std::string> table_key_columns_names() const {
            std::vector<std::string> res;
            this->visit_table_primary_key([this, &res](auto& primaryKey) {
                res = this->table_key_columns_names(primaryKey);
            });
            return res;
        }

        std::vector<std::string> primary_key_column_names() const {
            using pkcol_index_sequence = col_index_sequence_with<elements_type, is_primary_key>;

            if constexpr (pkcol_index_sequence::size() > 0) {
                return create_from_tuple<std::vector<std::string>>(this->elements,
                                                                   pkcol_index_sequence{},
                                                                   &column_identifier::name);
            } else {
                return this->table_key_columns_names();
            }
        }

        template<class L>
        void for_each_primary_key_column(L&& lambda) const {
            iterate_tuple(this->elements,
                          col_index_sequence_with<elements_type, is_primary_key>{},
                          call_as_template_base<column_field>([&lambda](const auto& column) {
                              lambda(column.member_pointer);
                          }));
            this->visit_table_primary_key([&lambda](auto& primaryKey) {
                iterate_tuple(primaryKey.columns, lambda);
            });
        }

        template<class... Args>
        std::vector<std::string> table_key_columns_names(const primary_key_t<Args...>& primaryKey) const {
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
    };

    template<class... Cs, class G, class S>
    bool table_primary_key_contains(const insertable_table_definition<Cs...>& definition,
                                    const column_field<G, S>& column) {
        bool res = false;
        definition.visit_table_primary_key([&column, &res](auto& primaryKey) {
            using colrefs_tuple = decltype(primaryKey.columns);
            using same_type_index_sequence =
                filter_tuple_sequence_t<colrefs_tuple,
                                        check_if_is_type<member_field_type_t<G>>::template fn,
                                        member_field_type_t>;
            iterate_tuple(primaryKey.columns, same_type_index_sequence{}, [&res, &column](auto& memberPointer) {
                if (compare_fields(memberPointer, column.member_pointer) ||
                    compare_fields(memberPointer, column.setter)) {
                    res = true;
                }
            });
        });
        return res;
    }

    template<class... Cs, class G, class S>
    bool is_single_table_primary_key(const insertable_table_definition<Cs...>& definition,
                                     const column_field<G, S>& column) {
        bool res = false;
        definition.visit_table_primary_key([&column, &res](auto& primaryKey) {
            // note: use `decltype(primaryKey)` instead of `decltype(primaryKey.columns)` otherwise msvc 141 chokes on the `if constexpr` below
            using colrefs_tuple = columns_tuple_t<polyfill::remove_cvref_t<decltype(primaryKey)>>;
            if constexpr (std::tuple_size<colrefs_tuple>::value != 1) {
                return;
            } else {
                auto& memberPointer = std::get<0>(primaryKey.columns);
                if (compare_fields(memberPointer, column.member_pointer) ||
                    compare_fields(memberPointer, column.setter)) {
                    res = true;
                }
            }
        });
        return res;
    }

    /**
     *  Mixin for a base table, providing methods used to access a mapped object's members.
     *  
     *  Implementation note: it is provided as a mixin to reduce the number of involved template parameters,
     *  which is possible in C++23 mode for 'getters'.
     */
#ifdef SQLITE_ORM_DEDUCING_THIS_SUPPORTED
    template<class O>
#else
    template<class O, class Definition>
#endif
    struct mapped_object_mixin {
        using object_type = O;

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

        template<class M, class... Cs, satisfies<is_setter, M> = true>
        const member_field_type_t<M>*
#ifdef SQLITE_ORM_DEDUCING_THIS_SUPPORTED
        object_field_value(this const table_definition<Cs...>& self, const object_type& object, M memberPointer) {
            using elements_type = elements_type_t<table_definition<Cs...>>;
#else
        object_field_value(const object_type& object, M memberPointer) const {
            using elements_type = elements_type_t<Definition>;
            auto& self = static_cast<const Definition&>(*this);

#endif
            using field_type = member_field_type_t<M>;
            const field_type* res = nullptr;
            iterate_tuple(self.elements,
                          col_index_sequence_with_field_type<elements_type, field_type>{},
                          call_as_template_base<column_field>([&res, &memberPointer, &object](const auto& column) {
                              if (compare_fields(column.setter, memberPointer)) {
                                  res = &polyfill::invoke(column.member_pointer, object);
                              }
                          }));
            return res;
        }
    };
}
