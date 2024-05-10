#pragma once

#ifdef SQLITE_ORM_WITH_CTE
#include <string>
#include <vector>
#include <functional>  //  std::reference_wrapper
#include <system_error>
#endif

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "member_traits/member_traits.h"
#include "error_code.h"
#include "alias.h"
#include "select_constraints.h"
#include "serializer_context.h"

#ifdef SQLITE_ORM_WITH_CTE
namespace sqlite_orm {
    namespace internal {
        // collecting column names utilizes the statement serializer
        template<class T, class C>
        auto serialize(const T& t, const C& context);

        inline void unquote_identifier(std::string& identifier) {
            if(!identifier.empty()) {
                constexpr char quoteChar = '"';
                constexpr char sqlEscaped[] = {quoteChar, quoteChar};
                identifier.erase(identifier.end() - 1);
                identifier.erase(identifier.begin());
                for(size_t pos = 0; (pos = identifier.find(sqlEscaped, pos, 2)) != identifier.npos; ++pos) {
                    identifier.erase(pos, 1);
                }
            }
        }

        inline void unquote_or_erase(std::string& name) {
            constexpr char quoteChar = '"';
            if(name.front() == quoteChar) {
                unquote_identifier(name);
            } else {
                // unaliased expression - see 3. below
                name.clear();
            }
        }

        template<class T, class SFINAE = void>
        struct cte_column_names_collector {
            using expression_type = T;

            // Compound statements are never passed in by db_objects_for_expression()
            static_assert(!is_compound_operator_v<T>);

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& t, const Ctx& context) const {
                auto newContext = context;
                newContext.skip_table_name = true;
                std::string columnName = serialize(t, newContext);
                if(columnName.empty()) {
                    throw std::system_error{orm_error_code::column_not_found};
                }
                unquote_or_erase(columnName);
                return {std::move(columnName)};
            }
        };

        template<class T, class Ctx>
        std::vector<std::string> get_cte_column_names(const T& t, const Ctx& context) {
            cte_column_names_collector<T> collector;
            return collector(t, context);
        }

        template<class As>
        struct cte_column_names_collector<As, match_specialization_of<As, as_t>> {
            using expression_type = As;

            template<class Ctx>
            std::vector<std::string> operator()(const expression_type& /*expression*/, const Ctx& /*context*/) const {
                return {alias_extractor<alias_type_t<As>>::extract()};
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
                auto& table = pick_table<T>(context.db_objects);

                std::vector<std::string> columnNames;
                columnNames.reserve(size_t(table.template count_of<is_column>()));

                table.for_each_column([&columnNames](const column_identifier& column) {
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

        // No CTE for object expressions.
        template<class Object>
        struct cte_column_names_collector<Object, match_if<is_struct, Object>> {
            static_assert(polyfill::always_false_v<Object>, "Repacking columns in a subselect is not allowed.");
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

                    if constexpr(polyfill::is_specialization_of_v<value_type, as_t>) {
                        columnNames.push_back(alias_extractor<alias_type_t<value_type>>::extract());
                    } else {
                        std::string columnName = serialize(m, newContext);
                        if(!columnName.empty()) {
                            columnNames.push_back(std::move(columnName));
                        } else {
                            throw std::system_error{orm_error_code::column_not_found};
                        }
                        unquote_or_erase(columnNames.back());
                    }
                });
                return columnNames;
            }
        };

        template<typename Ctx, typename E, typename ExplicitColRefs, satisfies_is_specialization_of<E, select_t> = true>
        std::vector<std::string>
        collect_cte_column_names(const E& sel, const ExplicitColRefs& explicitColRefs, const Ctx& context) {
            // 1. determine column names from subselect
            std::vector<std::string> columnNames = get_cte_column_names(sel.col, context);

            // 2. override column names from cte expression
            if(size_t n = std::tuple_size_v<ExplicitColRefs>) {
                if(n != columnNames.size()) {
                    throw std::system_error{orm_error_code::column_not_found};
                }

                size_t idx = 0;
                iterate_tuple(explicitColRefs, [&idx, &columnNames, &context](auto& colRef) {
                    using ColRef = polyfill::remove_cvref_t<decltype(colRef)>;

                    if constexpr(polyfill::is_specialization_of_v<ColRef, alias_holder>) {
                        columnNames[idx] = alias_extractor<type_t<ColRef>>::extract();
                    } else if constexpr(std::is_member_pointer<ColRef>::value) {
                        using O = table_type_of_t<ColRef>;
                        if(auto* columnName = find_column_name<O>(context.db_objects, colRef)) {
                            columnNames[idx] = *columnName;
                        } else {
                            // relaxed: allow any member pointer as column reference
                            columnNames[idx] = typeid(ColRef).name();
                        }
                    } else if constexpr(polyfill::is_specialization_of_v<ColRef, column_t>) {
                        columnNames[idx] = colRef.name;
                    } else if constexpr(std::is_same_v<ColRef, std::string>) {
                        if(!colRef.empty()) {
                            columnNames[idx] = colRef;
                        }
                    } else if constexpr(std::is_same_v<ColRef, polyfill::remove_cvref_t<decltype(std::ignore)>>) {
                        if(columnNames[idx].empty()) {
                            columnNames[idx] = std::to_string(idx + 1);
                        }
                    } else {
                        static_assert(polyfill::always_false_v<ColRef>, "Invalid explicit column reference specified");
                    }
                    ++idx;
                });
            }

            // 3. fill in blanks with numerical column identifiers
            {
                for(size_t i = 0, n = columnNames.size(); i < n; ++i) {
                    if(columnNames[i].empty()) {
                        columnNames[i] = std::to_string(i + 1);
                    }
                }
            }

            return columnNames;
        }
    }
}
#endif
