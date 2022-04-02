#pragma once

#include <type_traits>
#include <tuple>
#include <string>
#include <vector>

#include "column_result.h"
#include "column_expression.h"
#include "select_constraints.h"
#include "table.h"
#include "cte_types.h"
#include "cte_column_names_collector.h"
#include "storage_lookup.h"

namespace sqlite_orm {
    namespace internal {

        // F = field_type
        template<typename Label, typename Expression, typename F>
        struct create_cte_mapper {
            using type = subselect_mapper<Label, Expression, F>;
        };

        template<typename Label, typename Expression, typename... Fs>
        struct create_cte_mapper<Label, Expression, std::tuple<Fs...>> {
            using type = subselect_mapper<Label, Expression, Fs...>;
        };

        template<typename Label, typename Expression, typename... Fs>
        using create_cte_mapper_t = typename create_cte_mapper<Label, Expression, Fs...>::type;

        template<typename O, size_t CI>
        struct create_cte_column {
            using cte_object_type = O;
            using field_type = std::tuple_element_t<CI, typename O::fields_type>;
            using getter_type = cte_getter_t<O, CI>;
            using setter_type = cte_setter_t<O, CI>;

            using type = column_t<cte_object_type, field_type, getter_type, setter_type>;
        };

        /**
         *  Metafunction to create a CTE column_t type.
         */
        template<typename O, size_t CI>
        using create_cte_column_t = typename create_cte_column<O, CI>::type;

        template<typename Mapper, typename IdxSeq>
        struct create_cte_table;

        template<typename Label, typename Expression, typename... Fs, size_t... CIs>
        struct create_cte_table<subselect_mapper<Label, Expression, Fs...>, std::index_sequence<CIs...>> {
            using cte_mapper_type = subselect_mapper<Label, Expression, Fs...>;
            using table_type =
                table_t<cte_mapper_type, true, create_cte_column_t<cte_object_type_t<cte_mapper_type>, CIs>...>;

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

        template<typename Mapper, typename S, typename CTE, size_t... CIs>
        create_cte_table_t<Mapper, typename Mapper::index_sequence>
        make_cte_table_using_column_indices(const S& impl, const CTE& cte, std::index_sequence<CIs...>) {
            using O = cte_object_type_t<Mapper>;
            using context_type = serializer_context<S>;
            context_type context{impl};

            std::vector<std::string> columnNames =
                collect_cte_column_names(get_cte_driving_subselect(cte.subselect), cte.explicitColumnNames, context);

            return create_cte_table_t<Mapper, typename Mapper::index_sequence>{
                cte.label(),
                std::make_tuple(
                    make_column<>(move(columnNames.at(CIs)), cte_getter_v<O, CIs>, cte_setter_v<O, CIs>)...)};
        }

        template<typename Mapper, typename S, typename CTE>
        create_cte_table_t<Mapper, typename Mapper::index_sequence> make_cte_table(const S& impl, const CTE& cte) {
            return make_cte_table_using_column_indices<Mapper>(impl, cte, typename Mapper::index_sequence{});
        }

        template<typename S, typename... CTEs, size_t Ii>
        decltype(auto) make_recursive_cte_storage_using_table_indices(const S& impl,
                                                                      const common_table_expressions<CTEs...>& cte,
                                                                      std::index_sequence<Ii>) {
            using cte_type = std::tuple_element_t<Ii, common_table_expressions<CTEs...>>;
            using subselect_type = cte_driving_subselect_t<expression_type_t<cte_type>>;
            using mapper_type = create_cte_mapper_t<cte_label_type_t<cte_type>,
                                                    column_expression_of_t<S, subselect_type>,
                                                    column_result_of_t<S, subselect_type>>;
            static_assert(cte_type::explicit_column_count == 0 ||
                              cte_type::explicit_column_count == mapper_type::index_sequence::size(),
                          "Number of explicit columns of common table expression doesn't match the number of columns "
                          "in the subselect.");

            auto tbl = make_cte_table<mapper_type>(impl, get<Ii>(cte));

            return storage_impl_cat(impl, std::move(tbl));
        }

        template<typename S, typename... CTEs, size_t Ii, size_t Ij, size_t... In>
        decltype(auto) make_recursive_cte_storage_using_table_indices(const S& impl,
                                                                      const common_table_expressions<CTEs...>& cte,
                                                                      std::index_sequence<Ii, Ij, In...>) {
            using cte_type = std::tuple_element_t<Ii, common_table_expressions<CTEs...>>;
            using subselect_type = cte_driving_subselect_t<expression_type_t<cte_type>>;
            using mapper_type = create_cte_mapper_t<cte_label_type_t<cte_type>,
                                                    column_expression_of_t<S, subselect_type>,
                                                    column_result_of_t<S, subselect_type>>;
            static_assert(cte_type::explicit_column_count == 0 ||
                              cte_type::explicit_column_count == mapper_type::index_sequence::size(),
                          "Number of explicit columns of common table expression doesn't match the number of columns "
                          "in the subselect.");

            auto tbl = make_cte_table<mapper_type>(impl, get<Ii>(cte));

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
