#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::remove_const, std::is_member_pointer, std::true_type, std::false_type
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_element
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cxx_functional_polyfill.h"
#include "../functional/mpl.h"
#include "../functional/index_sequence_util.h"
#include "../tuple_helper/tuple_filter.h"
#include "../tuple_helper/tuple_traits.h"
#include "../tuple_helper/tuple_iteration.h"
#include "../tuple_helper/tuple_transformer.h"
#include "../member_traits/member_traits.h"
#include "../field_of.h"
#include "../type_traits.h"
#include "../constraints.h"
#include "../table_info.h"
#include "table_base.h"
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

        /**
         *  Table definition.
         */
        template<class O, bool WithoutRowId, class... Cs>
        struct table_t : table_base, mapped_object_base<O, Cs...> {
            using base_type = mapped_object_base<O, Cs...>;
            using object_type = typename base_type::object_type;
            using elements_type = typename base_type::elements_type;

            static constexpr bool is_without_rowid_v = WithoutRowId;
            using is_without_rowid = polyfill::bool_constant<is_without_rowid_v>;

            table_t<O, true, Cs...> without_rowid() const {
                return {this->name, this->elements};
            }

            const basic_generated_always::storage_type*
            find_column_generated_storage_type([[maybe_unused]] const std::string& name) const {
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

                if constexpr (pkcol_index_sequence::size() > 0) {
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

            std::vector<table_xinfo> get_table_info() const;
        };

        template<class T>
        struct is_table : std::false_type {};

        template<class O, bool W, class... Cs>
        struct is_table<table_t<O, W, Cs...>> : std::true_type {};

        template<class M>
        struct virtual_table : table_base, M {
            using module_type = M;
            using object_type = typename module_type::object_type;
            using elements_type = typename module_type::elements_type;

            static constexpr bool is_without_rowid_v = false;
            using is_without_rowid = polyfill::bool_constant<is_without_rowid_v>;

            const module_type& module() const {
                return *this;
            }
        };

#if SQLITE_VERSION_NUMBER >= 3009000
        template<class T, class... Cs>
        struct fts5_module : mapped_columns_mixin<Cs...> {
            using base_type = mapped_columns_mixin<Cs...>;
            using object_type = T;
            using elements_type = typename base_type::elements_type;
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
                    if (compare_fields(memberPointer, column.member_pointer) ||
                        compare_fields(memberPointer, column.setter)) {
                        res = true;
                    }
                });
            });
            return res;
        }

        template<class M, class G, class S>
        bool exists_in_composite_primary_key(const virtual_table<M>& /*virtualTable*/,
                                             const column_field<G, S>& /*column*/) {
            return false;
        }
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::fts5_module<T, Cs...> using_fts5(Cs... columns) {
        static_assert(polyfill::conjunction_v<internal::is_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(columns)...)});
    }

    template<class T, class... Cs>
    internal::fts5_module<T, Cs...> using_fts5(Cs... columns) {
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
    internal::virtual_table<M> make_virtual_table(std::string name, M module) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::move(name), std::move(module)});
    }
}
