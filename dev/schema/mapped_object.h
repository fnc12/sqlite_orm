#pragma once

#include <type_traits>  //  std::is_member_pointer
#include <string>  //  std::string
#include <tuple>  //  std::tuple_element
#include <utility>  //  std::move

#include "../functional/cxx_universal.h"  //  ::size_t
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
         *  Base for a mapped schema object aka table, view.
         */
        template<class O, class... Cs>
        struct mapped_object_t : basic_table {
            using object_type = O;
            using elements_type = std::tuple<Cs...>;

            elements_type elements;

#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            mapped_object_t(std::string name, elements_type elements) :
                basic_table{std::move(name)}, elements{std::move(elements)} {}
#endif

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
                                  if(compare_any(column.setter, memberPointer)) {
                                      res = &polyfill::invoke(column.member_pointer, object);
                                  }
                              }));
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
        };
    }
}
