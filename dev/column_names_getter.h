#pragma once

#include <type_traits>  //  std::is_base_of
#include <string>  //  std::string
#include <vector>  //  std::vector
#include <functional>  //  std::reference_wrapper
#include <system_error>
#include <utility>  //  std::move

#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_iteration.h"
#include "error_code.h"
#include "mapped_type_proxy.h"
#include "alias_traits.h"
#include "select_constraints.h"
#include "storage_lookup.h"  //  pick_table
#include "serializer_context.h"
#include "util.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class C>
        auto serialize(const T& t, const C& context);

        template<class T, class Ctx>
        std::vector<std::string>& collect_table_column_names(std::vector<std::string>& collectedExpressions,
                                                             bool definedOrder,
                                                             const Ctx& context) {
            if(definedOrder) {
                auto& table = pick_table<mapped_type_proxy_t<T>>(context.db_objects);
                collectedExpressions.reserve(collectedExpressions.size() + table.template count_of<is_column>());
                table.for_each_column([qualified = !context.skip_table_name,
                                       &tableName = table.name,
                                       &collectedExpressions](const column_identifier& column) {
                    if(is_alias<T>::value) {
                        collectedExpressions.push_back(quote_identifier(alias_extractor<T>::extract()) + "." +
                                                       quote_identifier(column.name));
                    } else if(qualified) {
                        collectedExpressions.push_back(quote_identifier(tableName) + "." +
                                                       quote_identifier(column.name));
                    } else {
                        collectedExpressions.push_back(quote_identifier(column.name));
                    }
                });
            } else {
                collectedExpressions.reserve(collectedExpressions.size() + 1);
                if(is_alias<T>::value) {
                    collectedExpressions.push_back(quote_identifier(alias_extractor<T>::extract()) + ".*");
                } else if(!context.skip_table_name) {
                    const basic_table& table = pick_table<mapped_type_proxy_t<T>>(context.db_objects);
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
            std::vector<std::string>& operator()(const E& t, const Ctx& context) {
                auto columnExpression = serialize(t, context);
                if(columnExpression.empty()) {
                    throw std::system_error{orm_error_code::column_not_found};
                }
                this->collectedExpressions.reserve(this->collectedExpressions.size() + 1);
                this->collectedExpressions.push_back(std::move(columnExpression));
                return this->collectedExpressions;
            }

            template<class T, class Ctx>
            std::vector<std::string>& operator()(const std::reference_wrapper<T>& expression, const Ctx& context) {
                return (*this)(expression.get(), context);
            }

            template<class T, class Ctx>
            std::vector<std::string>& operator()(const asterisk_t<T>& expression, const Ctx& context) {
                return collect_table_column_names<T>(this->collectedExpressions, expression.defined_order, context);
            }

            template<class T, class Ctx>
            std::vector<std::string>& operator()(const object_t<T>& expression, const Ctx& context) {
                return collect_table_column_names<T>(this->collectedExpressions, expression.defined_order, context);
            }

            template<class... Args, class Ctx>
            std::vector<std::string>& operator()(const columns_t<Args...>& cols, const Ctx& context) {
                this->collectedExpressions.reserve(this->collectedExpressions.size() + cols.count);
                iterate_tuple(cols.columns, [this, &context](auto& colExpr) {
                    (*this)(colExpr, context);
                });
                // note: `capacity() > size()` can occur in case `asterisk_t<>` does spell out the columns in defined order
                if(tuple_has_template<typename columns_t<Args...>::columns_type, asterisk_t>::value &&
                   this->collectedExpressions.capacity() > this->collectedExpressions.size()) {
                    this->collectedExpressions.shrink_to_fit();
                }
                return this->collectedExpressions;
            }

            template<class T, class... Args, class Ctx>
            std::vector<std::string>& operator()(const struct_t<T, Args...>& cols, const Ctx& context) {
                this->collectedExpressions.reserve(this->collectedExpressions.size() + cols.count);
                iterate_tuple(cols.columns, [this, &context](auto& colExpr) {
                    (*this)(colExpr, context);
                });
                // note: `capacity() > size()` can occur in case `asterisk_t<>` does spell out the columns in defined order
                if(tuple_has_template<typename struct_t<T, Args...>::columns_type, asterisk_t>::value &&
                   this->collectedExpressions.capacity() > this->collectedExpressions.size()) {
                    this->collectedExpressions.shrink_to_fit();
                }
                return this->collectedExpressions;
            }

            std::vector<std::string> collectedExpressions;
        };

        template<class T, class Ctx>
        std::vector<std::string> get_column_names(const T& t, const Ctx& context) {
            column_names_getter serializer;
            return serializer(t, context);
        }
    }
}
