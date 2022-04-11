#pragma once

#include <type_traits>
#include <tuple>
#include <string>
#include <vector>

#include "column_result.h"
#include "column_expression.h"
#include "select_constraints.h"
#include "table.h"
#include "alias.h"
#include "cte_types.h"
#include "cte_column_names_collector.h"
#include "storage_lookup.h"

namespace sqlite_orm {
    namespace internal {

        // F = field_type
        template<typename Label, typename Expression, typename RefExpressions, typename F>
        struct create_cte_mapper {
            using type = subselect_mapper<Label, Expression, RefExpressions, F>;
        };

        template<typename Label, typename Expression, typename RefExpressions, typename... Fs>
        struct create_cte_mapper<Label, Expression, RefExpressions, std::tuple<Fs...>> {
            using type = subselect_mapper<Label, Expression, RefExpressions, Fs...>;
        };

        template<typename Label, typename Expression, typename RefExpressions, typename... Fs>
        using create_cte_mapper_t = typename create_cte_mapper<Label, Expression, RefExpressions, Fs...>::type;

        // aliased column expressions, explicit or implicitly numbered
        template<typename Mapper, size_t CI, typename SFINAE = void>
        struct create_cte_column {
            using As = std::tuple_element_t<CI, typename Mapper::colref_expressions_tuple>;
            using field_type = std::tuple_element_t<CI, typename Mapper::fields_type>;
            using object_type = aliased_field<alias_type_t<As>, field_type>;

            using type = column_t<object_type,
                                  field_type,
                                  const field_type& (object_type::*)() const,
                                  void (object_type::*)(field_type)>;

            template<typename Tpl>
            static type make_column(std::string name, const Tpl& /*colRefs*/) {
                return sqlite_orm::make_column<>(move(name), &object_type::field);
            }
        };

        // F O::*
        template<typename Mapper, size_t CI>
        struct create_cte_column<
            Mapper,
            CI,
            std::enable_if_t<polyfill::disjunction_v<
                is_field_member_pointer<std::tuple_element_t<CI, typename Mapper::colref_expressions_tuple>>,
                is_getter<std::tuple_element_t<CI, typename Mapper::colref_expressions_tuple>>>>> {
            using F = std::tuple_element_t<CI, typename Mapper::colref_expressions_tuple>;
            using object_type = typename field_member_traits<F>::object_type;
            using field_type = typename field_member_traits<F>::field_type;

            using type = column_t<object_type,
                                  field_type,
                                  const field_type& (object_type::*)() const,
                                  void (object_type::*)(field_type)>;

            template<typename Tpl>
            static type make_column(std::string name, const Tpl& colRefs) {
                return sqlite_orm::make_column<>(move(name), get<CI>(colRefs));
            }
        };

        /**
         *  Metafunction to create a CTE column_t type.
         */
        template<typename Mapper, size_t CI>
        using create_cte_column_t = typename create_cte_column<Mapper, CI>::type;

        template<typename Mapper, typename IdxSeq>
        struct create_cte_table;

        template<typename Label, typename Expression, typename... Fs, size_t... CIs>
        struct create_cte_table<subselect_mapper<Label, Expression, Fs...>, std::index_sequence<CIs...>> {
            using cte_mapper_type = subselect_mapper<Label, Expression, Fs...>;
            using table_type = table_t<cte_mapper_type, true, create_cte_column_t<cte_mapper_type, CIs>...>;

            using type = table_type;
        };

        /**
         *  Metafunction to create a CTE table_t type.
         */
        template<typename Mapper, typename IdxSeq>
        using create_cte_table_t = typename create_cte_table<Mapper, IdxSeq>::type;

        /**
         *  Concatenate newly created tables with those from an existing storage implementation,
         *  forming a new storage implementation object.
         */
        template<typename... Ts, typename... CTETables>
        storage_impl<CTETables..., Ts...> storage_impl_cat(const storage_impl<Ts...>& storage,
                                                           CTETables&&... cteTables) {
            return {std::forward<CTETables>(cteTables)..., pick_impl<Ts>(storage).table...};
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

        template<typename Select>
        using cte_driving_subselect_t =
            polyfill::remove_cvref_t<decltype(get_cte_driving_subselect(std::declval<Select>()))>;

        template<class C, size_t... Idx>
        auto get_fields_of_columns(const C& coldef, std::index_sequence<Idx...>) {
            return std::make_tuple(get<Idx>(coldef).member_pointer...);
        }

        // any expression -> numeric column alias
        template<class S, class E, size_t Idx = 0>
        auto extract_colref_expressions(const S& /*impl*/, const E& /*col*/, std::index_sequence<Idx> = {})
            -> std::tuple<alias_holder<decltype(n_to_colalias<Idx>())>> {
            return {};
        }

        // expression_t<>
        template<class S, class E, size_t Idx = 0>
        auto extract_colref_expressions(const S& impl, const expression_t<E>& col, std::index_sequence<Idx> s = {}) {
            return extract_colref_expressions(impl, col.value, s);
        }

        // F O::* (field) -> field
        template<class S, class F, class O, size_t Idx = 0>
        auto extract_colref_expressions(const S& /*impl*/, F O::*col, std::index_sequence<Idx> = {}) {
            return std::make_tuple(col);
        }

        // as_t<> (aliased expression) -> alias_holder
        template<class S, class A, class E, size_t Idx = 0>
        std::tuple<alias_holder<A>>
        extract_colref_expressions(const S& /*impl*/, const as_t<A, E>& /*col*/, std::index_sequence<Idx> = {}) {
            return {};
        }

        // alias_holder<> (colref) -> alias_holder
        template<class S, class A, size_t Idx = 0>
        std::tuple<alias_holder<A>>
        extract_colref_expressions(const S& /*impl*/, const alias_holder<A>& /*col*/, std::index_sequence<Idx> = {}) {
            return {};
        }

        // column_pointer<>
        template<class S, class Label, class F, size_t Idx = 0>
        auto extract_colref_expressions(const S& impl,
                                        const column_pointer<Label, F>& col,
                                        std::index_sequence<Idx> s = {}) {
            return extract_colref_expressions(impl, col.field, s);
        }

        // column expression tuple
        template<class S, class... Args, size_t... Idx>
        auto extract_colref_expressions(const S& impl, const std::tuple<Args...>& cols, std::index_sequence<Idx...>) {
            return std::tuple_cat(extract_colref_expressions(impl, get<Idx>(cols), std::index_sequence<Idx>{})...);
        }

        // columns_t<>
        template<class S, class... Args>
        auto extract_colref_expressions(const S& impl, const columns_t<Args...>& cols) {
            return extract_colref_expressions(impl, cols.columns, std::index_sequence_for<Args...>{});
        }

        // asterisk_t<> -> fields
        template<class S, class O>
        auto extract_colref_expressions(const S& impl, const asterisk_t<O>& /*col*/) {
            using timpl_type = storage_pick_impl_t<S, O>;
            using elements_t = typename timpl_type::table_type::elements_type;
            using column_idxs = filter_tuple_sequence_t<elements_t, is_column>;

            auto& tImpl = pick_impl<O>(impl);
            return get_fields_of_columns(tImpl.table.elements, column_idxs{});
        }

        template<class S, class E, class... Args>
        decltype(auto) extract_colref_expressions(const S& /*impl*/,
                                                  const select_t<E, Args...>& /*subSelect*/) = delete;

        template<typename S, typename SubSelect>
        using extracted_colref_expressions_t =
            polyfill::remove_cvref_t<decltype(extract_colref_expressions(std::declval<S>(),
                                                                         std::declval<SubSelect>().col))>;

        template<typename Mapper, typename S, typename CTE, size_t... CIs>
        create_cte_table_t<Mapper, typename Mapper::index_sequence>
        make_cte_table_using_column_indices(const S& impl, const CTE& cte, std::index_sequence<CIs...>) {
            using context_type = serializer_context<S>;
            using cte_type = CTE;

            context_type context{impl};
            std::string tableName = alias_extractor<cte_label_type_t<cte_type>>::extract(std::true_type{});
            auto subSelect = get_cte_driving_subselect(cte.subselect);
            auto colRefs = extract_colref_expressions(impl, subSelect.col);
            std::vector<std::string> columnNames =
                collect_cte_column_names(subSelect, cte.explicitColumnNames, context);

            return create_cte_table_t<Mapper, typename Mapper::index_sequence>{
                move(tableName),
                std::make_tuple(create_cte_column<Mapper, CIs>::make_column(move(columnNames.at(CIs)), colRefs)...)};
        }

        template<typename S, typename CTE>
        auto make_cte_table(const S& impl, const CTE& cte) {
            using cte_type = CTE;
            using subselect_type = cte_driving_subselect_t<expression_type_t<cte_type>>;
            using mapper_type = create_cte_mapper_t<cte_label_type_t<cte_type>,
                                                    column_expression_of_t<S, subselect_type>,
                                                    extracted_colref_expressions_t<S, subselect_type>,
                                                    column_result_of_t<S, subselect_type>>;
            static_assert(cte_type::explicit_column_count == 0 ||
                              cte_type::explicit_column_count == mapper_type::index_sequence::size(),
                          "Number of explicit columns of common table expression doesn't match the number of columns "
                          "in the subselect.");

            return make_cte_table_using_column_indices<mapper_type>(impl, cte, typename mapper_type::index_sequence{});
        }

        template<typename S, typename... CTEs, size_t Ii>
        decltype(auto) make_recursive_cte_storage_using_table_indices(const S& impl,
                                                                      const common_table_expressions<CTEs...>& cte,
                                                                      std::index_sequence<Ii>) {
            auto tbl = make_cte_table(impl, get<Ii>(cte));

            return storage_impl_cat(impl, std::move(tbl));
        }

        template<typename S, typename... CTEs, size_t Ii, size_t Ij, size_t... In>
        decltype(auto) make_recursive_cte_storage_using_table_indices(const S& impl,
                                                                      const common_table_expressions<CTEs...>& cte,
                                                                      std::index_sequence<Ii, Ij, In...>) {
            auto tbl = make_cte_table(impl, get<Ii>(cte));

            return make_recursive_cte_storage_using_table_indices(
                // Because CTEs can depend on their predecessor we recursively pass in a new storage object
                storage_impl_cat(impl, std::move(tbl)),
                cte,
                std::index_sequence<Ij, In...>{});
        }

        /**
         *  Return const storage_impl of storage_t.
         */
        template<class S, class E, satisfies<is_storage, S> = true>
        decltype(auto) storage_for_expression(S& storage, const E&) {
            return obtain_const_impl(storage);
        }

        /**
         *  Return new storage_impl for CTE expressions.
         */
        template<class S, class E, class... CTEs, satisfies<is_storage, S> = true>
        decltype(auto) storage_for_expression(S& storage, const with_t<E, CTEs...>& e) {
            return make_recursive_cte_storage_using_table_indices(obtain_const_impl(storage),
                                                                  e.cte,
                                                                  std::index_sequence_for<CTEs...>{});
        }
    }
}
