#pragma once

#include "functional/cxx_core_features.h"
#ifdef SQLITE_ORM_WITH_CTE
#include <type_traits>
#include <tuple>
#include <string>
#include <vector>

#include "functional/cxx_universal.h"
#include "is_base_of_template.h"
#include "table_type_of.h"
#include "column_result.h"
#include "select_constraints.h"
#include "table.h"
#include "alias.h"
#include "cte_types.h"
#include "cte_column_names_collector.h"
#endif
#include "column_expression.h"
#include "storage_lookup.h"

namespace sqlite_orm {
    namespace internal {

#ifdef SQLITE_ORM_WITH_CTE
        // F = field_type
        template<typename Label,
                 typename ExplicitColRefs,
                 typename Expression,
                 typename SubselectColRefs,
                 typename FinalColRefs,
                 typename F>
        struct create_cte_mapper {
            using type = subselect_mapper<Label, ExplicitColRefs, Expression, SubselectColRefs, FinalColRefs, F>;
        };

        // std::tuple<Fs...>
        template<typename Label,
                 typename ExplicitColRefs,
                 typename Expression,
                 typename SubselectColRefs,
                 typename FinalColRefs,
                 typename... Fs>
        struct create_cte_mapper<Label,
                                 ExplicitColRefs,
                                 Expression,
                                 SubselectColRefs,
                                 FinalColRefs,
                                 std::tuple<Fs...>> {
            using type = subselect_mapper<Label, ExplicitColRefs, Expression, SubselectColRefs, FinalColRefs, Fs...>;
        };

        template<typename Label,
                 typename ExplicitColRefs,
                 typename Expression,
                 typename SubselectColRefs,
                 typename FinalColRefs,
                 typename Result>
        using create_cte_mapper_t =
            typename create_cte_mapper<Label, ExplicitColRefs, Expression, SubselectColRefs, FinalColRefs, Result>::
                type;

        // aliased column expressions, explicit or implicitly numbered
        template<typename F, typename ColRef, satisfies_is_specialization_of<ColRef, alias_holder> = true>
        static auto make_cte_column(std::string name, const ColRef& /*finalColRef*/) {
            using object_type = aliased_field<type_t<ColRef>, F>;

            return sqlite_orm::make_column<>(move(name), &object_type::field);
        }

        // F O::*
        template<typename F, typename ColRef, satisfies<std::is_member_pointer, ColRef> = true>
        static auto make_cte_column(std::string name, const ColRef& finalColRef) {
            using object_type = table_type_of_t<ColRef>;
            using column_type = column_t<ColRef, empty_setter>;

            return column_type{move(name), finalColRef, empty_setter{}};
        }

        /**
         *  Concatenate newly created tables with those from an existing storage implementation,
         *  forming a new storage implementation object.
         */
        template<typename... Ts, typename... CTETables>
        db_objects_tuple<CTETables..., Ts...> storage_db_objects_cat(const db_objects_tuple<Ts...>& storage,
                                                                     CTETables&&... cteTables) {
            return {std::forward<CTETables>(cteTables)..., pick_table<Ts>(storage)...};
        }

        /**
         *  This function returns the expression contained in a subselect that is relevant for
         *  creating the definition of a CTE table.
         *  Because CTEs can recursively refer to themselves in a compound statement, parsing
         *  the whole compound statement would lead to compiler errors if a column_pointer<>
         *  can't be resolved. Therefore, at the time of building a CTE table, we are only
         *  interested in the column results of the left expression.
         */
        template<class Select>
        decltype(auto) get_cte_driving_subselect(const Select& subSelect);

        /**
         *  Return given select expression.
         */
        template<class Select>
        decltype(auto) get_cte_driving_subselect(const Select& subSelect) {
            return subSelect;
        }

        /**
         *  Return left expression of compound statement.
         */
        template<class Compound,
                 class... Args,
                 std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
        decltype(auto) get_cte_driving_subselect(const select_t<Compound, Args...>& subSelect) {
            return subSelect.col.left;
        }

        /**
         *  Return a tuple of member pointers of all columns
         */
        template<class C, size_t... Idx>
        auto get_fields_of_columns(const C& coldef, std::index_sequence<Idx...>) {
            return std::make_tuple(get<Idx>(coldef).member_pointer...);
        }

        // any expression -> numeric column alias
        template<class DBOs, class E, size_t Idx = 0>
        auto extract_colref_expressions(const DBOs& /*dbObjects*/, const E& /*col*/, std::index_sequence<Idx> = {})
            -> std::tuple<alias_holder<decltype(n_to_colalias<Idx>())>> {
            return {};
        }

        // expression_t<>
        template<class DBOs, class E, size_t Idx = 0>
        auto
        extract_colref_expressions(const DBOs& dbObjects, const expression_t<E>& col, std::index_sequence<Idx> s = {}) {
            return extract_colref_expressions(dbObjects, col.value, s);
        }

        // F O::* (field/getter) -> field/getter
        template<class DBOs, class F, class O, size_t Idx = 0>
        auto extract_colref_expressions(const DBOs& /*dbObjects*/, F O::*col, std::index_sequence<Idx> = {}) {
            return std::make_tuple(col);
        }

        // as_t<> (aliased expression) -> alias_holder
        template<class DBOs, class A, class E, size_t Idx = 0>
        std::tuple<alias_holder<A>> extract_colref_expressions(const DBOs& /*dbObjects*/,
                                                               const as_t<A, E>& /*col*/,
                                                               std::index_sequence<Idx> = {}) {
            return {};
        }

        // alias_holder<> (colref) -> alias_holder
        template<class DBOs, class A, size_t Idx = 0>
        std::tuple<alias_holder<A>> extract_colref_expressions(const DBOs& /*dbObjects*/,
                                                               const alias_holder<A>& /*col*/,
                                                               std::index_sequence<Idx> = {}) {
            return {};
        }

        // column_pointer<>
        template<class DBOs, class Label, class F, size_t Idx = 0>
        auto extract_colref_expressions(const DBOs& dbObjects,
                                        const column_pointer<Label, F>& col,
                                        std::index_sequence<Idx> s = {}) {
            return extract_colref_expressions(dbObjects, col.field, s);
        }

        // column expression tuple
        template<class DBOs, class... Args, size_t... Idx>
        auto extract_colref_expressions(const DBOs& dbObjects,
                                        const std::tuple<Args...>& cols,
                                        std::index_sequence<Idx...>) {
            return std::tuple_cat(extract_colref_expressions(dbObjects, get<Idx>(cols), std::index_sequence<Idx>{})...);
        }

        // columns_t<>
        template<class DBOs, class... Args>
        auto extract_colref_expressions(const DBOs& dbObjects, const columns_t<Args...>& cols) {
            return extract_colref_expressions(dbObjects, cols.columns, std::index_sequence_for<Args...>{});
        }

        // asterisk_t<> -> fields
        template<class DBOs, class O>
        auto extract_colref_expressions(const DBOs& dbObjects, const asterisk_t<O>& /*col*/) {
            using table_type = storage_pick_table_t<O, DBOs>;
            using elements_t = typename table_type::elements_type;
            using column_idxs = filter_tuple_sequence_t<elements_t, is_column>;

            auto& table = pick_table<O>(dbObjects);
            return get_fields_of_columns(table.elements, column_idxs{});
        }

        template<class DBOs, class E, class... Args>
        void extract_colref_expressions(const DBOs& /*dbObjects*/, const select_t<E, Args...>& /*subSelect*/) = delete;
        template<class DBOs,
                 class Compound,
                 std::enable_if_t<is_base_of_template_v<Compound, compound_operator>, bool> = true>
        void extract_colref_expressions(const DBOs& /*dbObjects*/, const Compound& /*subSelect*/) = delete;

        /*
         *  Depending on ExplicitColRef's type returns either the explicit column reference
         *  or the expression's column reference otherwise.
         */
        template<typename DBOs, typename SubselectColRef, typename ExplicitColRef>
        auto determine_cte_colref(const DBOs& /*dbObjects*/,
                                  const SubselectColRef& subselectColRef,
                                  const ExplicitColRef& explicitColRef) {
            if constexpr(polyfill::is_specialization_of_v<ExplicitColRef, alias_holder>) {
                return explicitColRef;
            } else if constexpr(std::is_member_pointer<ExplicitColRef>::value) {
                return explicitColRef;
            } else if constexpr(std::is_base_of_v<column_identifier, ExplicitColRef>) {
                return explicitColRef.member_pointer;
            } else if constexpr(std::is_same_v<ExplicitColRef, std::string>) {
                return subselectColRef;
            } else if constexpr(std::is_same_v<ExplicitColRef, polyfill::remove_cvref_t<decltype(std::ignore)>>) {
                return subselectColRef;
            } else {
                static_assert(polyfill::always_false_v<ExplicitColRef>, "Invalid explicit column reference specified");
            }
        }

        template<typename DBOs, typename SubselectColRefs, typename ExplicitColRefs, size_t... Idx>
        auto determine_cte_colrefs([[maybe_unused]] const DBOs& dbObjects,
                                   const SubselectColRefs& subselectColRefs,
                                   [[maybe_unused]] const ExplicitColRefs& explicitColRefs,
                                   std::index_sequence<Idx...>) {
            if constexpr(std::tuple_size_v < ExplicitColRefs >> 0) {
                static_assert(
                    (!internal::is_builtin_numeric_column_alias_v<
                         alias_holder_type_or_none_t<std::tuple_element_t<Idx, ExplicitColRefs>>> &&
                     ...),
                    "Numeric column aliases are reserved for referencing columns locally within a single CTE.");

                return std::tuple{
                    determine_cte_colref(dbObjects, get<Idx>(subselectColRefs), get<Idx>(explicitColRefs))...};
            } else {
                return subselectColRefs;
            }
        }

        template<typename Mapper, typename DBOs, typename ColRefs, size_t... CIs>
        auto make_cte_table_using_column_indices(const DBOs& /*dbObjects*/,
                                                 std::string tableName,
                                                 std::vector<std::string> columnNames,
                                                 const ColRefs& finalColRefs,
                                                 std::index_sequence<CIs...>) {
            return make_table<Mapper>(
                move(tableName),
                make_cte_column<std::tuple_element_t<CIs, typename Mapper::fields_type>>(move(columnNames.at(CIs)),
                                                                                         get<CIs>(finalColRefs))...);
        }

        template<typename DBOs, typename CTE>
        auto make_cte_table(const DBOs& dbObjects, const CTE& cte) {
            using cte_type = CTE;

            auto subSelect = get_cte_driving_subselect(cte.subselect);

            using subselect_type = decltype(subSelect);
            using column_results = column_result_of_t<DBOs, subselect_type>;
            using index_sequence = std::make_index_sequence<std::tuple_size_v<tuplify_t<column_results>>>;
            static_assert(cte_type::explicit_colref_count == 0 ||
                              cte_type::explicit_colref_count == index_sequence::size(),
                          "Number of explicit columns of common table expression doesn't match the number of columns "
                          "in the subselect.");

            using context_type = serializer_context<DBOs>;

            std::string tableName = alias_extractor<cte_label_type_t<cte_type>>::extract();
            auto subselectColRefs = extract_colref_expressions(dbObjects, subSelect.col);
            const auto& finalColRefs =
                determine_cte_colrefs(dbObjects, subselectColRefs, cte.explicitColumns, index_sequence{});

            context_type context{dbObjects};
            std::vector<std::string> columnNames = collect_cte_column_names(subSelect, cte.explicitColumns, context);

            using mapper_type = create_cte_mapper_t<cte_label_type_t<cte_type>,
                                                    typename cte_type::explicit_colrefs_tuple,
                                                    column_expression_of_t<DBOs, subselect_type>,
                                                    decltype(subselectColRefs),
                                                    polyfill::remove_cvref_t<decltype(finalColRefs)>,
                                                    column_results>;
            return make_cte_table_using_column_indices<mapper_type>(dbObjects,
                                                                    move(tableName),
                                                                    move(columnNames),
                                                                    finalColRefs,
                                                                    index_sequence{});
        }

        template<typename DBOs, typename... CTEs, size_t Ii>
        decltype(auto) make_recursive_cte_storage_using_table_indices(const DBOs& dbObjects,
                                                                      const common_table_expressions<CTEs...>& cte,
                                                                      std::index_sequence<Ii>) {
            auto tbl = make_cte_table(dbObjects, get<Ii>(cte));

            return storage_db_objects_cat(dbObjects, std::move(tbl));
        }

        template<typename DBOs, typename... CTEs, size_t Ii, size_t Ij, size_t... In>
        decltype(auto) make_recursive_cte_storage_using_table_indices(const DBOs& dbObjects,
                                                                      const common_table_expressions<CTEs...>& cte,
                                                                      std::index_sequence<Ii, Ij, In...>) {
            auto tbl = make_cte_table(dbObjects, get<Ii>(cte));

            return make_recursive_cte_storage_using_table_indices(
                // Because CTEs can depend on their predecessor we recursively pass in a new storage object
                storage_db_objects_cat(dbObjects, std::move(tbl)),
                cte,
                std::index_sequence<Ij, In...>{});
        }

        /**
         *  Return new DBOs for CTE expressions.
         */
        template<class S, class E, class... CTEs, satisfies<is_storage, S> = true>
        decltype(auto) storage_for_expression(S& storage, const with_t<E, CTEs...>& e) {
            return make_recursive_cte_storage_using_table_indices(obtain_db_objects(storage),
                                                                  e.cte,
                                                                  std::index_sequence_for<CTEs...>{});
        }
#endif

        /**
         *  Return DBOs of storage_t.
         */
        template<class S, class E, satisfies<is_storage, S> = true>
        decltype(auto) storage_for_expression(S& storage, const E&) {
            return obtain_db_objects(storage);
        }
    }
}