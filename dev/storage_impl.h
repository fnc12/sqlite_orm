#pragma once

#include <string>  //  std::string
#include <sqlite3.h>
#include <sstream>  //  std::stringstream
#include <type_traits>  //  std::forward, std::is_same
#include <vector>  //  std::vector
#include <typeindex>  //  std::type_index

#include "static_magic.h"
#include "type_traits.h"
#include "constraints.h"
#include "table_info.h"
#include "sync_schema_result.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        struct storage_impl_base {

            bool table_exists(const std::string& tableName, sqlite3* db) const;

            void rename_table(sqlite3* db, const std::string& oldName, const std::string& newName) const;

            static bool calculate_remove_add_columns(std::vector<table_info*>& columnsToAdd,
                                                     std::vector<table_info>& storageTableInfo,
                                                     std::vector<table_info>& dbTableInfo);
        };

        /**
         *  This is a generic implementation. Used as a tail in storage_impl inheritance chain
         */
        template<class... Ts>
        struct storage_impl;

        template<class H, class... Ts>
        struct storage_impl<H, Ts...> : public storage_impl<Ts...> {
            using super = storage_impl<Ts...>;
            using self = storage_impl<H, Ts...>;
            using table_type = H;

            storage_impl(H h, Ts... ts) : super(std::forward<Ts>(ts)...), table(std::move(h)) {}

            table_type table;

            template<class L>
            void for_each(const L& l) const {
                this->super::for_each(l);
                l(*this);
            }

#if SQLITE_VERSION_NUMBER >= 3006019

            /**
             *  Returns foreign keys count in table definition
             */
            int foreign_keys_count() const {
                auto res = 0;
                iterate_tuple(this->table.elements, [&res](auto& c) {
                    if(is_foreign_key<typename std::decay<decltype(c)>::type>::value) {
                        ++res;
                    }
                });
                return res;
            }

#endif

            std::string find_table_name(const std::type_index& ti) const {
                std::type_index thisTypeIndex{typeid(std::conditional_t<std::is_void<label_of_or_void_t<H>>::value,
                                                                        object_type_t<H>,
                                                                        label_of_or_void_t<H>>)};

                if(thisTypeIndex == ti) {
                    return this->table.name;
                } else {
                    return this->super::find_table_name(ti);
                }
            }

            /**
             *  Copies current table to another table with a given **name**.
             *  Performs CREATE TABLE %name% AS SELECT %this->table.columns_names()% FROM &this->table.name%;
             */
            void
            copy_table(sqlite3* db, const std::string& name, const std::vector<table_info*>& columnsToIgnore) const;
        };

        template<>
        struct storage_impl<> : storage_impl_base {

            std::string find_table_name(const std::type_index&) const {
                return {};
            }

            template<class L>
            void for_each(const L&) const {}

            int foreign_keys_count() const {
                return 0;
            }
        };
    }
}

#include "static_magic.h"
#include "select_constraints.h"

// interface functions
namespace sqlite_orm {
    namespace internal {

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        auto lookup_table(const S& strg) {
            const auto& tImpl = find_impl<Lookup>(strg);
            return static_if<std::is_same<decltype(tImpl), const storage_impl<>&>{}>(
                [](const storage_impl<>&) {
                    return nullptr;
                },
                [](const auto& tImpl) {
                    return &tImpl.table;
                })(tImpl);
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        std::string lookup_table_name(const S& strg) {
            const auto& tImpl = find_impl<Lookup>(strg);
            return static_if<std::is_same<decltype(tImpl), const storage_impl<>&>{}>(
                [](const storage_impl<>&) {
                    return std::string{};
                },
                [](const auto& tImpl) {
                    return tImpl.table.name;
                })(tImpl);
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        const std::string& get_table_name(const S& strg) {
            return pick_impl<Lookup>(strg).table.name;
        }

        /**
         *  Find column name by its type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& strg, F O::*field) {
            return pick_impl<O>(strg).table.find_column_name(field);
        }

        /**
         *  Materialize column pointer:
         *  1. by explicit object type and member pointer.
         *  2. by label type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) materialize_column_pointer(const S&, const column_pointer<O, F>& cp) {
            return cp.field;
        }

        /**
         *  Materialize column pointer:
         *  3. by label type and alias_holder<>.
         */
        template<class Label, class ColAlias, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) materialize_column_pointer(const S&,
                                                            const column_pointer<Label, alias_holder<ColAlias>>&) {
            using timpl_type = storage_pick_impl_t<S, Label>;
            using cte_mapper_type = storage_cte_mapper_type_t<timpl_type>;

            // filter all column references [`alias_holder<>`]
            using alias_types_tuple =
                transform_tuple_t<typename cte_mapper_type::final_colrefs_tuple, alias_holder_type_or_none>;

            // lookup index in alias_types_tuple by Alias
            constexpr auto ColIdx = tuple_index_of_v<ColAlias, alias_types_tuple>;
            static_assert(ColIdx != -1, "No such column mapped into the CTE.");

            return &aliased_field<ColAlias, std::tuple_element_t<ColIdx, cte_mapper_type::fields_type>>;
        }

        /**
         *  Find column name by:
         *  1. by explicit object type and member pointer.
         *  2. by label type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& strg, const column_pointer<O, F>& cp) {
            auto field = materialize_column_pointer(strg, cp);
            return pick_impl<O>(strg).table.find_column_name(field);
        }

        /**
         *  Find column name by:
         *  3. by label type and alias_holder<>.
         */
        template<class Label, class ColAlias, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) find_column_name(const S& s, const column_pointer<Label, alias_holder<ColAlias>>&) {
            using timpl_type = storage_pick_impl_t<S, Label>;
            using cte_mapper_type = storage_cte_mapper_type_t<timpl_type>;
            using elements_t = typename timpl_type::table_type::elements_type;
            using column_idxs = filter_tuple_sequence_t<elements_t, is_column>;

            // note: even though the columns contain the [`aliased_field<>::*`] we perform the lookup using the column references.
            // filter all column references [`alias_holder<>`]
            using alias_types_tuple =
                transform_tuple_t<typename cte_mapper_type::final_colrefs_tuple, alias_holder_type_or_none>;

            // lookup index of ColAlias in alias_types_tuple
            static constexpr auto I = tuple_index_of_v<ColAlias, alias_types_tuple>;
            static_assert(I != -1, "No such column mapped into the CTE.");

            // note: we could "materialize" the alias to an `aliased_field<>::*` and use the regular `table_t<>::find_column_name()` mechanism;
            //       however we have the column index already.
            // lookup column in table_t<>'s elements
            constexpr size_t ColIdx = index_sequence_value(I, column_idxs{});
            auto& timpl = pick_impl<Label>(s);
            return &std::get<ColIdx>(timpl.table.elements).name;
        }

        /**
         *  Find column name by:
         *  4. by label type and index constant.
         */
        template<class Label, size_t I, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) find_column_name(const S& s,
                                                  const column_pointer<Label, polyfill::index_constant<I>>&) {
            using timpl_type = storage_pick_impl_t<S, Label>;
            using column_idxs = filter_tuple_sequence_t<typename timpl_type::table_type::elements_type, is_column>;

            // lookup column in table_t<>'s elements
            constexpr size_t ColIdx = index_sequence_value(I, column_idxs{});
            static_assert(ColIdx < column_idxs{}.size(), "No such column mapped into the CTE.");

            auto& timpl = pick_impl<Label>(s);
            return &std::get<ColIdx>(timpl.table.elements).name;
        }
    }
}
