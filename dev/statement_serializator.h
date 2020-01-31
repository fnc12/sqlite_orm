#pragma once

#include <sstream>  //  std::stringstream
#include <string>  //  std::string
#include <type_traits>  //  std::is_arithmetic, std::enable_if
#include <vector>  //  std::vector

#include "core_functions.h"
#include "constraints.h"
#include "conditions.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct statement_serializator;

        template<class T, class C>
        std::string serialize(const T &t, const C &context) {
            statement_serializator<T> serializator;
            return serializator(t, context);
        }

        template<class O, class F>
        struct statement_serializator<F O::*, void> {
            using statement_type = F O::*;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return context.impl.column_name(c);
            }
        };

        template<class R, class S, class... Args>
        struct statement_serializator<core_functions::core_function_t<R, S, Args...>, void> {
            using statement_type = core_functions::core_function_t<R, S, Args...>;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << "(";
                std::vector<std::string> args;
                using args_type = typename std::decay<decltype(c)>::type::args_type;
                args.reserve(std::tuple_size<args_type>::value);
                iterate_tuple(c.args, [&args, &context](auto &v) {
                    args.push_back(serialize(v, context));
                });
                for(size_t i = 0; i < args.size(); ++i) {
                    ss << args[i];
                    if(i < args.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")";
                return ss.str();
            }
        };

        template<>
        struct statement_serializator<constraints::autoincrement_t, void> {
            using statement_type = constraints::autoincrement_t;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return static_cast<std::string>(c);
            }
        };

        template<class... Cs>
        struct statement_serializator<constraints::primary_key_t<Cs...>, void> {
            using statement_type = constraints::primary_key_t<Cs...>;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                auto res = static_cast<std::string>(c);
                using columns_tuple = typename statement_type::columns_tuple;
                auto columnsCount = std::tuple_size<columns_tuple>::value;
                if(columnsCount) {
                    res += "(";
                    decltype(columnsCount) columnIndex = 0;
                    iterate_tuple(c.columns, [&context, &res, &columnIndex, columnsCount](auto &column) {
                        res += context.column_name(column);
                        if(columnIndex < columnsCount - 1) {
                            res += ", ";
                        }
                        ++columnIndex;
                    });
                    res += ")";
                }
                return res;
            }
        };

        template<>
        struct statement_serializator<constraints::unique_t, void> {
            using statement_type = constraints::unique_t;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return static_cast<std::string>(c);
            }
        };

        template<>
        struct statement_serializator<constraints::collate_t, void> {
            using statement_type = constraints::collate_t;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return static_cast<std::string>(c);
            }
        };

        template<class T>
        struct statement_serializator<constraints::default_t<T>, void> {
            using statement_type = constraints::default_t<T>;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return static_cast<std::string>(c) + " (" + serialize(c.value, context) + ")";
            }
        };

        template<>
        struct statement_serializator<std::string, void> {
            using statement_type = std::string;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return "\"" + c + "\"";
            }
        };

        template<>
        struct statement_serializator<const char *, void> {
            using statement_type = const char *;

            template<class C>
            std::string operator()(const char *c, const C &context) const {
                return std::string("'") + c + "'";
            }
        };

        template<class T>
        struct statement_serializator<constraints::check_t<T>, void> {
            using statement_type = constraints::check_t<T>;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return static_cast<std::string>(c) + " (" + serialize(c.expression, context) + ")";
            }
        };

        template<class T>
        struct statement_serializator<
            T,
            typename std::enable_if<is_base_of_template<T, conditions::binary_condition>::value>::type> {
            using statement_type = T;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                auto leftString = serialize(c.l, context);
                auto rightString = serialize(c.r, context);
                std::stringstream ss;
                ss << "(" << leftString << " " << static_cast<std::string>(c) << " " << rightString << ")";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializator<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
            using statement_type = T;

            template<class C>
            std::string operator()(const statement_type &t, const C &context) const {
                std::stringstream ss;
                ss << t;
                return ss.str();
            }
        };

        template<class L, class R, class... Ds>
        struct statement_serializator<binary_operator<L, R, Ds...>, void> {
            using statement_type = binary_operator<L, R, Ds...>;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                auto lhs = serialize(c.lhs, context);
                auto rhs = serialize(c.rhs, context);
                std::stringstream ss;
                ss << "(" << lhs << " " << static_cast<std::string>(c) << " " << rhs << ")";
                return ss.str();
            }
        };

    }
}
