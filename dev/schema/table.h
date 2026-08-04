#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::remove_const, std::true_type, std::false_type
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_element, std::make_tuple, std::get
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/mpl.h"
#include "../tuple_helper/tuple_filter.h"
#include "../tuple_helper/tuple_transformer.h"
#include "../type_traits.h"
#include "../table_constraints.h"
#include "../table_info.h"
#include "table_base.h"
#include "column.h"
#include "index.h"

namespace sqlite_orm::internal {
    template<class T>
    using is_base_table_element_or_constraint = mpl::invoke_t<mpl::disjunction<check_if<is_column>,
                                                                               check_if<is_primary_key>,
                                                                               check_if<is_foreign_key>,
                                                                               check_if_is_template<index_t>,
                                                                               check_if_is_template<unique_t>,
                                                                               check_if_is_template<check_t>>,
                                                              T>;

    /** 
     *  Encapsulates base table elements, i.e. columns and constraints for a base table,
     *  and provides additional methods to those of a generic table definition in order to deal with foreign key and generated columns.
     */
    template<class... Cs>
    struct base_table_definition : insertable_table_definition<Cs...> {
        using definition_base_type = insertable_table_definition<Cs...>;
        using elements_type = elements_type_t<definition_base_type>;

        const basic_generated_always::storage_type*
        find_column_generated_storage_type([[maybe_unused]] const std::string& name) const {
            const basic_generated_always::storage_type* result = nullptr;
#if SQLITE_VERSION_NUMBER >= 3031000
            iterate_tuple(
                this->elements,
                col_index_sequence_with<elements_type, is_generated_always>{},
                [&result, &name](auto& column) {
                    if (column.name != name) {
                        return;
                    }
                    using generated_op_index_sequence =
                        filter_tuple_sequence_t<std::remove_const_t<decltype(column.constraints)>, is_generated_always>;
                    constexpr size_t opIndex = index_sequence_value_at<0>(generated_op_index_sequence{});
                    result = &std::get<opIndex>(column.constraints)._storage;
                });
#endif
            return result;
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
    };

    /**
     *  Represents a base table, i.e. a table that actually stores data (as opposed to views and other virtual tables).
     */
    template<class O, class WithoutRowId, class... Cs>
    struct base_table : table_identifier,
                        base_table_definition<Cs...>,
#ifdef SQLITE_ORM_DEDUCING_THIS_SUPPORTED
                        mapped_object_mixin<O>
#else
                        mapped_object_mixin<O, table_definition<Cs...>>
#endif
    {
        using definition_type = base_table_definition<Cs...>;
        using object_type = O;
        using elements_type = typename definition_type::elements_type;
        using is_without_rowid = WithoutRowId;

        base_table<O, std::true_type, Cs...> without_rowid() const& {
            return {this->name, this->elements};
        }

        base_table<O, std::true_type, Cs...> without_rowid() && {
            return {std::move(this->name), std::move(this->elements)};
        }

        std::vector<table_xinfo> get_table_info() const;
    };

    template<class T>
    inline constexpr bool is_base_table_v = polyfill::is_specialization_of_v<T, base_table>;

    template<class T>
    using is_base_table = polyfill::bool_constant<is_base_table_v<T>>;

    template<class... Cs>
    constexpr void validate_base_table_definition() {
        static_assert(polyfill::conjunction_v<internal::is_base_table_element_or_constraint<Cs>...>,
                      "Incorrect base table elements or constraints");

        using elements_type = std::tuple<Cs...>;
        using pk_index_sequence = filter_tuple_sequence_t<elements_type, is_primary_key>;
        using pkcol_index_sequence = col_index_sequence_with<elements_type, is_primary_key>;

        static_assert(pk_index_sequence::size() + pkcol_index_sequence::size() <= 1,
                      "A base table can only have 1 primary key definition");
        if constexpr (pk_index_sequence::size() > 0) {
            constexpr size_t nTablePrimaryKeyColumns =
                nested_tuple_size_for_t<columns_tuple_t, elements_type, pk_index_sequence>::value;

            static_assert(nTablePrimaryKeyColumns > 0, "Table primary key definition must contain one column");
        }
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a base table.
     *  
     *  The mapped object type is determined implicitly from the first column definition.
     */
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::base_table<T, std::false_type, Cs...> make_table(std::string name, Cs... definition) {
        internal::validate_base_table_definition<Cs...>();
        return {std::move(name), std::tuple{std::forward<Cs>(definition)...}};
    }

    /**
     *  Factory function for a base table.
     *  
     *  The mapped object type is explicitly specified.
     */
    template<class T, class... Cs>
    internal::base_table<T, std::false_type, Cs...> make_table(std::string name, Cs... definition) {
        internal::validate_base_table_definition<Cs...>();
        return {std::move(name), std::tuple{std::forward<Cs>(definition)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Factory function for a base table.
     *  
     *  The mapped object type is explicitly specified.
     */
    template<orm_table_reference auto table, class... Cs>
    auto make_table(std::string name, Cs... definition) {
        return make_table<internal::auto_decay_table_ref_t<table>>(std::move(name), std::forward<Cs>(definition)...);
    }
#endif
}
