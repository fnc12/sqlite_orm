#pragma once

#include <string>
#include <vector>
#include <functional>  //  std::reference_wrapper
#include <system_error>

#include "cxx_polyfill.h"
#include "type_traits.h"
#include "error_code.h"
#include "select_constraints.h"

namespace sqlite_orm {
    namespace internal {
        // collecting column names utilizes the statement serializer
        template<class T, class C>
        std::string serialize(const T& t, const C& context);

        template<class T, class SFINAE = void>
        struct cte_column_names_collector {
            using expression_type = T;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& t, const Ctx& context) const {
                auto newContext = context;
                newContext.skip_table_name = true;
                std::string columnName = serialize(t, newContext);
                if(columnName.empty()) {
                    throw std::system_error(orm_error_code::column_not_found);
                }
                return {move(columnName)};
            }
        };

        template<class T, class Ctx>
        std::vector<std::string> get_cte_column_names(const T& t, const Ctx& context) {
            cte_column_names_collector<T> collector;
            return collector(t, context);
        }

        // For compound statements we just need to collect the column alias names of the left expression
        template<class Compound>
        struct cte_column_names_collector<Compound,
                                          std::enable_if_t<is_base_of_template<Compound, compound_operator>::value>> {
            using expression_type = Compound;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& t, const Ctx& context) const {
                auto newContext = context;
                newContext.skip_table_name = true;
                return get_cte_column_names(t.left.col, context);
            }
        };

        template<class As>
        struct cte_column_names_collector<As, match_specialization_of<As, as_t>> {
            using expression_type = As;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& /*expression*/, const Ctx& /*context*/) const {
                std::stringstream s;
                s << As::alias_type::get();
                return {s.str()};
            }
        };

        template<class Wrapper>
        struct cte_column_names_collector<Wrapper, match_specialization_of<Wrapper, std::reference_wrapper>> {
            using expression_type = Wrapper;

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
        struct cte_column_names_collector<Object, match_specialization_of<Object, object_t>> {
            static_assert(polyfill::always_false_v<Object>, "Selecting an object in a subselect is not allowed.");
        };

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
                    using value_type = polyfill::remove_cvref_t<decltype(m)>;

                    std::string columnName;
                    if constexpr(polyfill::is_specialization_of_v<value_type, as_t>) {
                        columnName = alias_extractor<typename value_type::alias_type>::extract();
                    } else {
                        columnName = serialize(m, newContext);
                    }
                    if(!columnName.empty()) {
                        columnNames.push_back(move(columnName));
                    } else {
                        throw std::system_error(orm_error_code::column_not_found);
                    }
                });
                return columnNames;
            }
        };

        template<typename Ctx, typename E, satisfies_is_specialization_of<E, select_t> = true>
        std::vector<std::string> collect_cte_column_names(const E& sel,
                                                          const std::vector<std::string>& explicitColumnNames,
                                                          const Ctx& context) {
            // 1. determine column names from subselect

            std::vector<std::string> columnNames = get_cte_column_names(sel.col, context);

            // unquote column names
            for(std::string& name: columnNames) {
                if(!name.empty() && name.front() == '"' && name.back() == '"') {
                    name.erase(name.end() - 1);
                    name.erase(name.begin());
                }
            }

            // 2. override column names from cte expression
            if(!explicitColumnNames.empty()) {
                if(explicitColumnNames.size() != columnNames.size()) {
                    throw std::system_error(orm_error_code::column_not_found);
                }

                for(size_t i = 0, n = explicitColumnNames.size(); i < n; ++i) {
                    if(!explicitColumnNames[i].empty()) {
                        columnNames[i] = explicitColumnNames[i];
                    }
                }
            }
            // 3. fill in blanks with numerical column names
            {
                for(size_t i = 0, n = columnNames.size(); i < n; ++i) {
                    if(columnNames[i].empty()) {
                        columnNames[i] = std::to_string(i);
                    }
                }
            }

            return columnNames;
        }
    }
}
