#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#include <string>  //  std::to_string
#include <vector>
#include <functional>  //  std::reference_wrapper
#include <system_error>
#include <type_traits>  //  std::is_member_pointer, std::is_same, std::remove_cvref
#include <utility>  //  std::move
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/type_traits.h"
#include "../tuple_helper/tuple_transformer.h"
#include "../error_code.h"
#include "../ast/alias.h"
#include "../vocabulary/node_traits.h"
#include "../vocabulary/node_algorithms.h"  // access_column_expression
#include "../schema/column_identifier.h"

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
namespace sqlite_orm::internal {
    // collecting column names utilizes the statement serializer
    template<class T, class Ctx>
    auto serialize(const T& t, const Ctx& context);

    inline void unquote_identifier(std::string& identifier) {
        if (!identifier.empty()) {
            constexpr char quoteChar = '"';
            constexpr char sqlEscaped[] = {quoteChar, quoteChar};
            identifier.erase(identifier.end() - 1);
            identifier.erase(identifier.begin());
            for (size_t pos = 0; (pos = identifier.find(sqlEscaped, pos, 2)) != identifier.npos; ++pos) {
                identifier.erase(pos, 1);
            }
        }
    }

    inline void unquote_or_erase(std::string& name) {
        constexpr char quoteChar = '"';
        if (name.front() == quoteChar) {
            unquote_identifier(name);
        } else {
            // unaliased expression - see 3. below
            name.clear();
        }
    }

    /**
     *  Collects the column names of a CTE's select: an alias' name, a column's name, or an empty name
     *  for an unaliased expression, to be numbered later on.
     */
    struct cte_column_names_collector {
        template<class E, class Ctx>
        SQLITE_ORM_STATIC_CALLOP std::vector<std::string> operator()(const E& expression,
                                                                     const Ctx& context) SQLITE_ORM_OR_CONST_CALLOP {
            // Compound statements are never passed in by db_objects_for_expression()
            static_assert(!is_compound_operator_v<E>);

            // ...
            if constexpr (polyfill::is_specialization_of_v<E, std::reference_wrapper>) {
                return operator()(access_column_expression(expression.get()), context);
            }
            // ...
            else if constexpr (is_asterisk_v<E>) {
                auto& table = pick_table<type_t<E>>(context.db_objects);

                using table_type = polyfill::remove_cvref_t<decltype(table)>;
                using column_index_sequence = col_index_sequence_of<elements_type_t<table_type>>;
                return create_from_tuple<std::vector<std::string>>(table.elements,
                                                                   column_index_sequence{},
                                                                   &column_identifier::name);
            }
            // No CTE for object expressions.
            else if constexpr (is_object_node_v<E>) {
                static_assert(polyfill::always_false_v<E>, "Selecting an object in a subselect is not allowed");
            }
            // No CTE for object expressions.
            else if constexpr (is_struct_v<E>) {
                static_assert(polyfill::always_false_v<E>, "Repacking columns in a subselect is not allowed");
            }
            // ...
            else if constexpr (is_columns_v<E>) {
                std::vector<std::string> columnNames;
                columnNames.reserve(size_t(expression.count));
                iterate_tuple(expression.columns, [&columnNames, &context](auto& column) {
                    columnNames.push_back(column_name(column, context));
                });
                return columnNames;
            }
            // ...
            else {
                return {column_name(expression, context)};
            }
        }

      private:
        /**
         *  The name of a single result column: an alias' name, or the unquoted serialization of the expression
         *  if it is a column, otherwise empty.
         */
        template<class E, class Ctx>
        static std::string column_name(const E& expression, const Ctx& context) {
            if constexpr (is_as_node_v<E>) {
                return alias_extractor<alias_type_t<E>>::extract();
            } else {
                auto newContext = context;
                newContext.omit_table_name = true;
                std::string columnName = serialize(expression, newContext);
                if (columnName.empty()) {
                    throw std::system_error{orm_error_code::column_not_found};
                }
                unquote_or_erase(columnName);
                return columnName;
            }
        }
    };

    template<class T, class Ctx>
    std::vector<std::string> collect_cte_column_names(const T& t, const Ctx& context) {
        return cte_column_names_collector{}(access_column_expression(t), context);
    }

    /**
     *  The column names of a CTE: those collected from its select, overridden by its explicit column list if any.
     */
    template<typename Ctx, typename E, typename ExplicitColRefs, satisfies<is_select, E> = true>
    std::vector<std::string> resolve_cte_column_names(const E& sel,
                                                      [[maybe_unused]] const ExplicitColRefs& explicitColRefs,
                                                      const Ctx& context) {
        // 1. collect the column names of the subselect
        std::vector<std::string> columnNames = collect_cte_column_names(sel.col, context);

        // 2. override column names from cte expression
        constexpr size_t nExplicitColumns = std::tuple_size_v<ExplicitColRefs>;
        if constexpr (nExplicitColumns > 0) {
            if (nExplicitColumns != columnNames.size()) {
                throw std::system_error{orm_error_code::column_not_found};
            }

            size_t idx = 0;
            iterate_tuple(explicitColRefs, [&idx, &columnNames, &context](auto& colRef) {
                using ColRef = polyfill::remove_cvref_t<decltype(colRef)>;

                if constexpr (is_alias_holder_v<ColRef>) {
                    columnNames[idx] = alias_extractor<type_t<ColRef>>::extract();
                } else if constexpr (std::is_member_pointer<ColRef>::value) {
                    using O = table_type_of_t<ColRef>;
                    if (auto* columnName = find_column_name<O>(context.db_objects, colRef)) {
                        columnNames[idx] = *columnName;
                    } else {
                        // relaxed: allow any member pointer as column reference
                        columnNames[idx] = typeid(ColRef).name();
                    }
                } else if constexpr (is_column_v<ColRef>) {
                    columnNames[idx] = colRef.name;
                } else if constexpr (std::is_same_v<ColRef, std::string>) {
                    if (!colRef.empty()) {
                        columnNames[idx] = colRef;
                    }
                } else if constexpr (std::is_same_v<ColRef, polyfill::remove_cvref_t<decltype(std::ignore)>>) {
                    if (columnNames[idx].empty()) {
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
#ifdef SQLITE_ORM_INITSTMT_RANGE_BASED_FOR_SUPPORTED
            for (size_t n = 1; std::string& name: columnNames) {
                if (name.empty()) {
                    name = std::to_string(n);
                }
                ++n;
            }
#else
            for (size_t i = 0, n = columnNames.size(); i < n; ++i) {
                if (columnNames[i].empty()) {
                    columnNames[i] = std::to_string(i + 1);
                }
            }
#endif
        }

        return columnNames;
    }
}
#endif
