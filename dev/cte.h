#pragma once

#include <type_traits>
#include <tuple>
#include <string>
#include <vector>
#include <system_error>

#include "cxx_polyfill.h"
#include "column_result.h"
#include "select_constraints.h"
#include "alias.h"
#include "table.h"
#include "cte_types.h"
#include "storage.h"
#include "statement_serializator.h"

namespace sqlite_orm {

    template<char C, char... Chars>
    struct cte_label {
        static constexpr char str[] = {C, Chars..., '\0'};

        static constexpr const char* label() {
            return str;
        }

        explicit operator std::string() const {
            return cte_label::label();
        }
    };

    using cte_1 = cte_label<'c', 't', 'e', '_', '1'>;
    using cte_2 = cte_label<'c', 't', 'e', '_', '2'>;
    using cte_3 = cte_label<'c', 't', 'e', '_', '3'>;
    using cte_4 = cte_label<'c', 't', 'e', '_', '4'>;
    using cte_5 = cte_label<'c', 't', 'e', '_', '5'>;
    using cte_6 = cte_label<'c', 't', 'e', '_', '6'>;
    using cte_7 = cte_label<'c', 't', 'e', '_', '7'>;
    using cte_8 = cte_label<'c', 't', 'e', '_', '8'>;
    using cte_9 = cte_label<'c', 't', 'e', '_', '9'>;

#if __cplusplus >= 201703L  // use of C++17 or higher
    namespace internal {
        constexpr size_t _10_pow(size_t n) {
            if(n == 0) {
                return 1;
            } else {
                return 10 * _10_pow(n - 1);
            }
        }

        template<class... Chars, size_t... Is>
        constexpr size_t n_from_literal(std::index_sequence<Is...>, Chars... chars) {
            return (((chars - '0') * _10_pow(sizeof...(Is) - 1u - Is /*reversed index sequence*/)) + ...);
        }
    }

    // index_constant<> from numeric literal
    template<char... Chars>
    [[nodiscard]] constexpr decltype(auto) operator"" _col() {
        return polyfill::index_constant<internal::n_from_literal(std::make_index_sequence<sizeof...(Chars)>{},
                                                                 Chars...)>{};
    }
#endif

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

        template<class T, class SFINAE = void>
        struct cte_column_names_collector {
            using expression_type = T;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& t, const Ctx& context) const {
                auto newContext = context;
                newContext.skip_table_name = true;
                std::string columnName = serialize(t, newContext);
                if(columnName.empty()) {
                    throw std::system_error(std::make_error_code(orm_error_code::column_not_found));
                }
                return {move(columnName)};
            }
        };

        template<class T, class Ctx>
        std::vector<std::string> get_cte_column_names(const T& t, const Ctx& context) {
            cte_column_names_collector<T> serializator;
            return serializator(t, context);
        }

        template<class As>
        struct cte_column_names_collector<As, match_specialization_of<As, as_t>> {
            using expression_type = As;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& /*expression*/, const Ctx& /*context*/) const {
                return {As::alias_type::get()};
            }
        };

        template<class RefWrapper>
        struct cte_column_names_collector<RefWrapper, match_specialization_of<RefWrapper, std::reference_wrapper>> {
            using expression_type = RefWrapper;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& expression, const Ctx& context) const {
                return get_cte_column_names(expression.get(), context);
            }
        };

        template<class Asterisk>
        struct cte_column_names_collector<Asterisk, match_specialization_of<Asterisk, asterisk_t>> {
            using expression_type = Asterisk;
            using T = typename Asterisk::type;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type&, const Ctx& context) const {
                auto& tImpl = pick_impl<T>(context.impl);

                std::vector<std::string> columnNames;
                columnNames.reserve(size_t(tImpl.table.elements_count));

                tImpl.table.for_each_column([&columnNames](const basic_column& column) {
                    columnNames.push_back(column.name);
                });
                return columnNames;
            }
        };

        // No CTE for object expressions.
        template<class Object>
        struct cte_column_names_collector<Object, match_specialization_of<Object, object_t>>;

        template<class Columns>
        struct cte_column_names_collector<Columns, match_specialization_of<Columns, columns_t>> {
            using expression_type = Columns;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& cols, const Ctx& context) const {
                std::vector<std::string> columnNames;
                columnNames.reserve(size_t(cols.count));
                auto newContext = context;
                newContext.skip_table_name = true;
                iterate_tuple(cols.columns, [&columnNames, &newContext](auto& m) {
                    std::string columnName;
                    if constexpr(polyfill::is_specialization_of_v<std::remove_cvref_t<decltype(m)>, as_t>) {
                        columnName = alias_extractor<typename std::remove_cvref_t<decltype(m)>::alias_type>::get();
                    } else {
                        columnName = serialize(m, newContext);
                    }
                    if(!columnName.empty()) {
                        columnNames.push_back(move(columnName));
                    } else {
                        throw std::system_error(std::make_error_code(orm_error_code::column_not_found));
                    }
                });
                return columnNames;
            }
        };

        template<typename Strg, typename T, typename... Args>
        std::vector<std::string> collect_cte_column_names(const Strg& storage, const select_t<T, Args...>& sel) {
            const serializator_context_builder<Strg> ctxBuilder(storage);
            return get_cte_column_names(sel.col, ctxBuilder());
        }

        template<typename O, typename Strg, typename CTE, size_t... CIs>
        create_cte_table_t<O, typename O::index_sequence>
        make_cte_table_using_column_indices(const Strg& storage, const CTE& cte, std::index_sequence<CIs...>) {
            std::vector<std::string> columnNames = collect_cte_column_names(storage, cte.expression);
            assert(columnNames.size() == O::index_sequence::size());
            // unquote
            for(std::string& name: columnNames) {
                if(!name.empty() && name.front() == '"' && name.back() == '"') {
                    name.erase(name.end() - 1);
                    name.erase(name.begin());
                }
            }

            return create_cte_table_t<O, typename O::index_sequence>{
                cte.label(),
                std::make_tuple(make_column<>(columnNames.at(CIs), cte_getter_v<O, CIs>, cte_setter_v<O, CIs>)...)};
        }

        template<typename O, typename Strg, typename CTE>
        create_cte_table_t<O, typename O::index_sequence> make_cte_table(const Strg& storage, const CTE& cte) {
            return make_cte_table_using_column_indices<O>(storage, cte, typename O::index_sequence{});
        }

        template<class Strg, class E, class... CTEs, size_t... TIs>
        decltype(auto) make_cte_storage_using_table_indices(const Strg& storage,
                                                            const with_t<E, CTEs...>& e,
                                                            std::index_sequence<TIs...>) {
            return storage_impl_cat(
                storage,
                make_cte_table<
                    create_column_results_t<typename CTEs::label_type,
                                            typename column_result_t<Strg, typename CTEs::expression_type>::type>>(
                    storage,
                    get<TIs>(e.cte))...);
        }

        template<class Strg, class E, class... CTEs>
        decltype(auto) make_cte_storage(const Strg& storage, const with_t<E, CTEs...>& e) {
            return make_cte_storage_using_table_indices(storage, e, std::index_sequence_for<CTEs...>{});
        }
    }
}
