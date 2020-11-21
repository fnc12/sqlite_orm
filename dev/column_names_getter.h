#pragma once

#include <string>  //  std::string
#include <vector>  //  std::vector
#include <functional>  //  std::reference_wrapper

#include "error_code.h"
#include "select_constraints.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class C>
        std::string serialize(const T& t, const C& context);

        template<class T, class SFINAE = void>
        struct column_names_getter {
            using expression_type = T;

            template<class C>
            std::vector<std::string> operator()(const expression_type& t, const C& context) {
                auto newContext = context;
                newContext.skip_table_name = false;
                auto columnName = serialize(t, newContext);
                if(columnName.length()) {
                    return {move(columnName)};
                } else {
                    throw std::system_error(std::make_error_code(orm_error_code::column_not_found));
                }
            }
        };

        template<class T, class C>
        std::vector<std::string> get_column_names(const T& t, const C& context) {
            column_names_getter<T> serializator;
            return serializator(t, context);
        }

        template<class T>
        struct column_names_getter<std::reference_wrapper<T>, void> {
            using expression_type = std::reference_wrapper<T>;

            template<class C>
            std::vector<std::string> operator()(const expression_type& expression, const C& context) {
                return get_column_names(expression.get(), context);
            }
        };

        template<class T>
        struct column_names_getter<asterisk_t<T>, void> {
            using expression_type = asterisk_t<T>;

            template<class C>
            std::vector<std::string> operator()(const expression_type&, const C&) {
                std::vector<std::string> res;
                res.push_back("*");
                return res;
            }
        };

        template<class T>
        struct column_names_getter<object_t<T>, void> {
            using expression_type = object_t<T>;

            template<class C>
            std::vector<std::string> operator()(const expression_type&, const C&) {
                std::vector<std::string> res;
                res.push_back("*");
                return res;
            }
        };

        template<class... Args>
        struct column_names_getter<columns_t<Args...>, void> {
            using expression_type = columns_t<Args...>;

            template<class C>
            std::vector<std::string> operator()(const expression_type& cols, const C& context) {
                std::vector<std::string> columnNames;
                columnNames.reserve(static_cast<size_t>(cols.count));
                auto newContext = context;
                newContext.skip_table_name = false;
                iterate_tuple(cols.columns, [&columnNames, &newContext](auto& m) {
                    auto columnName = serialize(m, newContext);
                    if(columnName.length()) {
                        columnNames.push_back(columnName);
                    } else {
                        throw std::system_error(std::make_error_code(orm_error_code::column_not_found));
                    }
                });
                return columnNames;
            }
        };

    }
}
