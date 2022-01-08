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
#include "storage.h"
#include "statement_serializator.h"

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
            template<template<typename...> typename Base, typename... Fs>
            Base<Fs...>& as_base_cast(Base<Fs...>& base) {
                return base;
            }
            template<template<typename...> typename Base, typename... Fs>
            const Base<Fs...>& as_base_cast(const Base<Fs...>& base) {
                return base;
            }
            template<template<typename...> typename Base, typename... Fs>
            Base<Fs...>&& as_base_cast(Base<Fs...>&& base) {
                return std::move(base);
            }
            template<template<typename...> typename Base, typename... Fs>
            const Base<Fs...>&& as_base_cast(const Base<Fs...>&& base) {
                return std::move(base);
            }

            /** @short Deduce template base specialization from a derived type */
            template<template<typename...> class Base, class Derived>
            using as_base_cast_t = decltype(as_base_cast<Base>(std::declval<Derived>()));

            template<template<typename...> class Base, class Derived, typename SFINAE = void>
            struct is_template_base_of : std::false_type {};

            template<template<typename...> class Base, class Derived>
            struct is_template_base_of<Base, Derived, polyfill::void_t<as_base_cast_t<Base, Derived>>>
                : std::true_type {};

            template<template<typename...> class Base, class Derived>
            using is_template_base_of_t = typename is_template_base_of<Base, Derived>::type;

            template<template<typename...> class Base, class Derived>
            SQLITE_ORM_INLINE_VAR constexpr bool is_template_base_of_v = is_template_base_of<Base, Derived>::value;
        }
    }
}

namespace sqlite_orm {

    template<char C, char... Cs>
    struct cte_label {
        static constexpr char str[] = {C, Cs..., '\0'};

        static const char* label() {
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

    namespace internal {
        /*
         *  Tuple data structure for CTEs
         */
        template<typename Label, typename... Fs>
        class column_results : std::tuple<Fs...> {
          public:
            using base = std::tuple<Fs...>;
            using index_sequence = std::index_sequence_for<Fs...>;
            using cte_label_type = Label;

            template<size_t I>
            decltype(auto) cget() const noexcept {
                return std::get<I>(polyfill::as_base_cast<std::tuple>(*this));
            }
            template<size_t I>
            void set(std::tuple_element_t<I, base> v) noexcept {
                std::get<I>(polyfill::as_base_cast<std::tuple>(*this)) = std::move(v);
            }
        };

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

        template<typename O, size_t I>
        struct create_cte_column {
            using object_type = O;
            using field_type = std::tuple_element_t<I, typename O::base>;
            using getter_type = decltype(&O::template cget<I>);
            using setter_type = decltype(&O::template set<I>);

            using type = column_t<object_type, field_type, getter_type, setter_type>;
        };

        template<typename O, size_t I>
        using create_cte_column_t = typename create_cte_column<O, I>::type;

        template<typename O, typename IdxSeq>
        struct create_cte_table;

        template<typename Label, typename... Fs, size_t... Is>
        struct create_cte_table<column_results<Label, Fs...>, std::index_sequence<Is...>> {
            using object_type = column_results<Label, Fs...>;
            using table_type = table_t<object_type, true, create_cte_column_t<object_type, Is>...>;

            using type = table_type;
        };

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

        template<class T, class E>
        struct cte_column_names_collector<as_t<T, E>, void> {
            using expression_type = as_t<T, E>;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& /*expression*/, const Ctx& /*context*/) const {
                return {T::get()};
            }
        };

        template<class T>
        struct cte_column_names_collector<std::reference_wrapper<T>, void> {
            using expression_type = std::reference_wrapper<T>;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& expression, const Ctx& context) const {
                return get_cte_column_names(expression.get(), context);
            }
        };

        template<class T>
        struct cte_column_names_collector<asterisk_t<T>, void> {
            using expression_type = asterisk_t<T>;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type&, const Ctx& context) const {
                auto& strgImpl = context.impl.get_impl<T>();

                std::vector<std::string> columnNames;
                columnNames.reserve(size_t(strgImpl.table.elements_count));

                strgImpl.table.for_each_column([&columnNames](const basic_column& column) {
                    columnNames.push_back(column.name);
                });
                return columnNames;
            }
        };

        template<class T>
        struct cte_column_names_collector<object_t<T>, void>;

        template<class... Args>
        struct cte_column_names_collector<columns_t<Args...>, void> {
            using expression_type = columns_t<Args...>;

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
        make_cte_table_with_column_indices(const Strg& storage, const CTE& cte, std::index_sequence<CIs...>) {
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
                std::make_tuple<>(
                    make_column<>(columnNames.at(CIs), &(O::template cget<CIs>), &(O::template set<CIs>))...)}
                .without_rowid();
        }

        template<typename O, typename Strg, typename CTE>
        create_cte_table_t<O, typename O::index_sequence> make_cte_table(const Strg& storage, const CTE& cte) {
            return make_cte_table_with_column_indices<O>(storage, cte, typename O::index_sequence{});
        }

        template<class Strg, class E, class... CTEs, size_t... TIs>
        decltype(auto) make_cte_storage_with_table_indices(const Strg& storage,
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
            return make_cte_storage_with_table_indices(storage, e, std::index_sequence_for<CTEs...>{});
        }
    }
}
