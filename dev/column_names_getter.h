#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of
#include <string>  //  std::string
#include <vector>  //  std::vector
#include <functional>  //  std::reference_wrapper
#include <system_error>
#include <utility>  //  std::move
#endif

#include "type_traits.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_iteration.h"
#include "error_code.h"
#include "mapped_type_proxy.h"
#include "alias_traits.h"
#include "storage_lookup.h"  //  pick_table
#include "util.h"  // quote_identifier
#include "vocabulary/node_traits.h"
#include "schema/column_identifier.h"
#include "schema/table_identifier.h"
#include "select_constraints.h"  // access_column_expression

namespace sqlite_orm::internal {
    template<class T, class Ctx>
    auto serialize(const T& t, const Ctx& context);

    template<class T, class Ctx>
    std::vector<std::string>&
    collect_table_column_names(std::vector<std::string>& collectedExpressions, bool definedOrder, const Ctx& context) {
        if (definedOrder) {
            auto& table = pick_table<mapped_type_proxy_t<T>>(context.db_objects);
            collectedExpressions.reserve(collectedExpressions.size() + table.template count_of<is_column>());
            table.for_each_column([qualified = !context.omit_table_name,
                                   &tableName = table.name,
                                   &collectedExpressions](const column_identifier& column) {
                if constexpr (is_alias_v<T>) {
                    collectedExpressions.push_back(quote_identifier(alias_extractor<T>::extract()) + "." +
                                                   quote_identifier(column.name));
                } else if (qualified) {
                    collectedExpressions.push_back(quote_identifier(tableName) + "." + quote_identifier(column.name));
                } else {
                    collectedExpressions.push_back(quote_identifier(column.name));
                }
            });
        } else {
            collectedExpressions.reserve(collectedExpressions.size() + 1);
            if constexpr (is_alias_v<T>) {
                collectedExpressions.push_back(quote_identifier(alias_extractor<T>::extract()) + ".*");
            } else if (!context.omit_table_name) {
                const table_identifier& table = pick_table<mapped_type_proxy_t<T>>(context.db_objects);
                collectedExpressions.push_back(quote_identifier(table.name) + ".*");
            } else {
                collectedExpressions.emplace_back("*");
            }
        }

        return collectedExpressions;
    }

    /** @short Column expression collector.
     */
    struct column_names_getter {
        /** 
         *  The default implementation simply serializes the passed argument.
         */
        template<class E, class Ctx>
        std::vector<std::string>& operator()(const E& expression, const Ctx& context) {
            // ...
            if constexpr (polyfill::is_specialization_of_v<E, std::reference_wrapper>) {
                return (*this)(expression.get(), context);
            }
            // ...
            else if constexpr (is_asterisk_v<E> || is_object_node_v<E>) {
                return collect_table_column_names<type_t<E>>(this->collectedExpressions,
                                                             expression.defined_order,
                                                             context);
            }
            // ...
            else if constexpr (is_columns_v<E> || is_struct_v<E>) {
                this->collectedExpressions.reserve(this->collectedExpressions.size() + expression.count);
                iterate_tuple(expression.columns, [this, &context](auto& colExpr) {
                    (*this)(colExpr, context);
                });
                // note: `capacity() > size()` can occur in case `asterisk_t<>` does spell out the columns in defined order
                if constexpr (tuple_has<typename E::columns_type, is_asterisk>::value) {
                    this->collectedExpressions.shrink_to_fit();
                }
                return this->collectedExpressions;
            }
            // ...
            else {
                std::string columnExpression = serialize(expression, context);
                if (columnExpression.empty()) {
                    throw std::system_error{orm_error_code::column_not_found};
                }
                this->collectedExpressions.reserve(this->collectedExpressions.size() + 1);
                this->collectedExpressions.push_back(std::move(columnExpression));
                return this->collectedExpressions;
            }
        }

        std::vector<std::string> collectedExpressions;
    };

    template<class T, class Ctx>
    std::vector<std::string> get_column_names(const T& expression, const Ctx& context) {
        column_names_getter serializer;
        return serializer(access_column_expression(expression), context);
    }
}
