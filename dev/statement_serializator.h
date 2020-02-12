#pragma once

#include <sstream>  //  std::stringstream
#include <string>  //  std::string
#include <type_traits>  //  std::is_arithmetic, std::enable_if
#include <vector>  //  std::vector
#include <algorithm>  //  std::iter_swap

#include "core_functions.h"
#include "constraints.h"
#include "conditions.h"
#include "column.h"

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
        struct statement_serializator<core_function_t<R, S, Args...>, void> {
            using statement_type = core_function_t<R, S, Args...>;

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

        template<class... Cs, class... Rs>
        struct statement_serializator<constraints::foreign_key_t<std::tuple<Cs...>, std::tuple<Rs...>>, void> {
            using statement_type = constraints::foreign_key_t<std::tuple<Cs...>, std::tuple<Rs...>>;

            template<class C>
            std::string operator()(const statement_type &fk, const C &context) const {
                std::stringstream ss;
                std::vector<std::string> columnNames;
                using columns_type_t = typename std::decay<decltype(fk)>::type::columns_type;
                constexpr const size_t columnsCount = std::tuple_size<columns_type_t>::value;
                columnNames.reserve(columnsCount);
                iterate_tuple(fk.columns, [&columnNames, &context](auto &v) {
                    columnNames.push_back(context.impl.column_name(v));
                });
                ss << "FOREIGN KEY(";
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ") REFERENCES ";
                std::vector<std::string> referencesNames;
                using references_type_t = typename std::decay<decltype(fk)>::type::references_type;
                constexpr const size_t referencesCount = std::tuple_size<references_type_t>::value;
                referencesNames.reserve(referencesCount);
                {
                    using first_reference_t = typename std::tuple_element<0, references_type_t>::type;
                    using first_reference_mapped_type = typename internal::table_type<first_reference_t>::type;
                    auto refTableName = context.impl.find_table_name(typeid(first_reference_mapped_type));
                    ss << refTableName;
                }
                iterate_tuple(fk.references, [&referencesNames, &context](auto &v) {
                    referencesNames.push_back(context.impl.column_name(v));
                });
                ss << "(";
                for(size_t i = 0; i < referencesNames.size(); ++i) {
                    ss << referencesNames[i];
                    if(i < referencesNames.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")";
                if(fk.on_update) {
                    ss << ' ' << static_cast<std::string>(fk.on_update) << " " << fk.on_update._action;
                }
                if(fk.on_delete) {
                    ss << ' ' << static_cast<std::string>(fk.on_delete) << " " << fk.on_delete._action;
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializator<constraints::check_t<T>, void> {
            using statement_type = constraints::check_t<T>;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                return static_cast<std::string>(c) + " " + serialize(c.expression, context);
            }
        };

        template<class O, class T, class G, class S, class... Op>
        struct statement_serializator<column_t<O, T, G, S, Op...>, void> {
            using statement_type = column_t<O, T, G, S, Op...>;

            template<class C>
            std::string operator()(const statement_type &c, const C &context) const {
                std::stringstream ss;
                ss << "'" << c.name << "' ";
                using column_type = typename std::decay<decltype(c)>::type;
                using field_type = typename column_type::field_type;
                using constraints_type = typename column_type::constraints_type;
                ss << type_printer<field_type>().print() << " ";
                {
                    std::vector<std::string> constraintsStrings;
                    constexpr const size_t constraintsCount = std::tuple_size<constraints_type>::value;
                    constraintsStrings.reserve(constraintsCount);
                    int primaryKeyIndex = -1;
                    int autoincrementIndex = -1;
                    int tupleIndex = 0;
                    iterate_tuple(
                        c.constraints,
                        [&constraintsStrings, &primaryKeyIndex, &autoincrementIndex, &tupleIndex, &context](auto &v) {
                            using constraint_type = typename std::decay<decltype(v)>::type;
                            constraintsStrings.push_back(serialize(v, context));
                            if(is_primary_key<constraint_type>::value) {
                                primaryKeyIndex = tupleIndex;
                            } else if(std::is_same<constraints::autoincrement_t, constraint_type>::value) {
                                autoincrementIndex = tupleIndex;
                            }
                            ++tupleIndex;
                        });
                    if(primaryKeyIndex != -1 && autoincrementIndex != -1 && autoincrementIndex < primaryKeyIndex) {
                        iter_swap(constraintsStrings.begin() + primaryKeyIndex,
                                  constraintsStrings.begin() + autoincrementIndex);
                    }
                    for(auto &str: constraintsStrings) {
                        ss << str << ' ';
                    }
                }
                if(c.not_null()) {
                    ss << "NOT NULL ";
                }
                return ss.str();
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
