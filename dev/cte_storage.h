#pragma once

#include <type_traits>
#include <tuple>
#include <string>
#include <vector>

#include "column_result.h"
#include "select_constraints.h"
#include "table.h"
#include "cte_types.h"
#include "cte_column_names_collector.h"
#include "storage_lookup.h"

namespace sqlite_orm {
    namespace internal {

        // F = field_type
        template<typename Label, typename F>
        struct create_column_results {
            using type = column_results<Label, F>;
        };

        template<typename Label, typename... Fs>
        struct create_column_results<Label, std::tuple<Fs...>> {
            using type = column_results<Label, Fs...>;
        };

        template<typename Label, typename... Fs>
        using create_column_results_t = typename create_column_results<Label, Fs...>::type;

        template<typename O, size_t CI>
        struct create_cte_column {
            using object_type = O;
            using field_type = std::tuple_element_t<CI, typename O::fields_type>;
            using getter_type = cte_getter_t<O, CI>;
            using setter_type = cte_setter_t<O, CI>;

            using type = column_t<object_type, field_type, getter_type, setter_type>;
        };

        /**
         *  Metafunction to create a CTE column_t type.
         */
        template<typename O, size_t CI>
        using create_cte_column_t = typename create_cte_column<O, CI>::type;

        template<typename O, typename IdxSeq>
        struct create_cte_table;

        template<typename Label, typename... Fs, size_t... CIs>
        struct create_cte_table<column_results<Label, Fs...>, std::index_sequence<CIs...>> {
            using object_type = column_results<Label, Fs...>;
            using table_type = table_t<object_type, true, create_cte_column_t<object_type, CIs>...>;

            using type = table_type;
        };

        /**
         *  Metafunction to create a CTE table_t type.
         */
        template<typename O, typename IdxSeq>
        using create_cte_table_t = typename create_cte_table<O, IdxSeq>::type;

        template<typename... Ts, typename... CTETables>
        storage_impl<CTETables..., Ts...> storage_impl_cat(const storage_t<Ts...>& storage, CTETables&&... cteTables) {
            return {std::forward<CTETables>(cteTables)..., pick_const_impl<object_type_t<Ts>>(storage).table...};
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
        decltype(auto) get_cte_driving_expression(const Select& subSelect);

        /**
         *  Return given select expression.
         */
        template<class Select>
        decltype(auto) get_cte_driving_expression(const Select& subSelect) {
            return subSelect;
        }

        /**
         *  Return left expression of compound statement.
         */
        template<class Compound,
                 class... Args,
                 std::enable_if_t<is_base_of_template<Compound, compound_operator>::value, bool> = true>
        decltype(auto) get_cte_driving_expression(const select_t<Compound, Args...>& subSelect) {
            return subSelect.col.left;
        }

        template<typename Select>
        using cte_driving_expression_t =
            std::remove_cvref_t<decltype(get_cte_driving_expression(std::declval<Select>()))>;

        template<typename O, typename S, typename CTE, size_t... CIs>
        create_cte_table_t<O, typename O::index_sequence>
        make_cte_table_using_column_indices(const S& storage, const CTE& cte, std::index_sequence<CIs...>) {
            using context_type = serializator_context<typename S::impl_type>;
            context_type context{obtain_const_impl(storage)};
            std::vector<std::string> columnNames =
                collect_cte_column_names(get_cte_driving_expression(cte.expression), cte.explicitColumnNames, context);

            return create_cte_table_t<O, typename O::index_sequence>{
                cte.label(),
                std::make_tuple(make_column<>(columnNames.at(CIs), cte_getter_v<O, CIs>, cte_setter_v<O, CIs>)...)};
        }

        template<typename O, typename S, typename CTE>
        create_cte_table_t<O, typename O::index_sequence> make_cte_table(const S& storage, const CTE& cte) {
            return make_cte_table_using_column_indices<O>(storage, cte, typename O::index_sequence{});
        }

        template<class S, class E, class... CTEs, size_t... TIs>
        decltype(auto) make_cte_storage_using_table_indices(const S& storage,
                                                            const with_t<E, CTEs...>& e,
                                                            std::index_sequence<TIs...>) {
            return storage_impl_cat(
                storage,
                make_cte_table<
                    create_column_results_t<label_type_t<CTEs>,
                                            column_result_of_t<S, cte_driving_expression_t<expression_type_t<CTEs>>>>>(
                    storage,
                    get<TIs>(e.cte))...);
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
            return make_cte_storage_using_table_indices(storage, e, std::index_sequence_for<CTEs...>{});
        }
    }
}
