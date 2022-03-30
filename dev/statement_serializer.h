#pragma once

#include <sstream>  //  std::stringstream
#include <string>  //  std::string
#include <type_traits>  //  std::enable_if, std::remove_pointer
#include <vector>  //  std::vector
#include <algorithm>  //  std::iter_swap
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <codecvt>  //  std::codecvt_utf8_utf16
#endif  //  SQLITE_ORM_OMITS_CODECVT
#include <cstddef>  // std::nullptr_t
#include <memory>

#include "start_macros.h"
#include "member_traits/is_getter.h"
#include "member_traits/is_setter.h"
#include "ast/upsert_clause.h"
#include "ast/excluded.h"
#include "ast/group_by.h"
#include "ast/into.h"
#include "core_functions.h"
#include "constraints.h"
#include "conditions.h"
#include "column.h"
#include "indexed_column.h"
#include "function.h"
#include "prepared_statement.h"
#include "rowid.h"
#include "pointer_value.h"
#include "type_printer.h"
#include "field_printer.h"
#include "table_name_collector.h"
#include "column_names_getter.h"
#include "order_by_serializer.h"
#include "statement_binder.h"
#include "values.h"
#include "triggers.h"
#include "table_type.h"
#include "index.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct statement_serializer;

        template<class T, class I>
        std::string serialize(const T& t, const serializer_context<I>& context) {
            statement_serializer<T> serializer;
            return serializer(t, context);
        }

        /**
         *  Serializer for bindable types.
         */
        template<class T>
        struct statement_serializer<T, match_if<is_bindable, T>> {
            using statement_type = T;

            template<class Ctx>
            std::string operator()(const T& statement, const Ctx& context) const {
                if(context.replace_bindable_with_question) {
                    return "?";
                } else {
                    return this->do_serialize(statement);
                }
            }

          private:
            template<class X,
                     std::enable_if_t<is_printable_v<X> && !std::is_base_of<std::string, X>::value
#ifndef SQLITE_ORM_OMITS_CODECVT
                                          && !std::is_base_of<std::wstring, X>::value
#endif
                                      ,
                                      bool> = true>
            std::string do_serialize(const X& c) const {
                static_assert(std::is_same<X, T>::value, "");

                // implementation detail: utilizing field_printer
                return field_printer<X>{}(c);
            }

            std::string do_serialize(const std::string& c) const {
                // implementation detail: utilizing field_printer
                return "'" + field_printer<std::string>{}(c) + "'";
            }

            std::string do_serialize(const char* c) const {
                return std::string("'") + c + "'";
            }
#ifndef SQLITE_ORM_OMITS_CODECVT
            std::string do_serialize(const std::wstring& c) const {
                // implementation detail: utilizing field_printer
                return "'" + field_printer<std::wstring>{}(c) + "'";
            }

            std::string do_serialize(const wchar_t* c) const {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return std::string("'") + converter.to_bytes(c) + "'";
            }
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
            std::string do_serialize(const std::string_view& c) const {
                return "'" + std::string(c) + "'";
            }
#ifndef SQLITE_ORM_OMITS_CODECVT
            std::string do_serialize(const std::wstring_view& c) const {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return "'" + converter.to_bytes(c.data(), c.data() + c.size()) + "'";
            }
#endif
#endif
            /**
             *  Specialization for binary data (std::vector<char>).
             */
            std::string do_serialize(const std::vector<char>& t) const {
                return "x'" + field_printer<std::vector<char>>{}(t) + "'";
            }

            template<class P, class PT, class D>
            std::string do_serialize(const pointer_binding<P, PT, D>&) const {
                // always serialize null (security reasons)
                return field_printer<std::nullptr_t>{}(nullptr);
            }
        };

        template<class F, class W>
        struct statement_serializer<filtered_aggregate_function<F, W>, void> {
            using statement_type = filtered_aggregate_function<F, W>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) {
                std::stringstream ss;
                ss << serialize(statement.function, context);
                ss << " FILTER (WHERE " << serialize(statement.where, context) << ")";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<excluded_t<T>, void> {
            using statement_type = excluded_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "excluded.";
                if(auto columnNamePointer = find_column_name(context.impl, statement.expression)) {
                    ss << "\"" << *columnNamePointer << "\"";
                } else {
                    throw std::system_error{orm_error_code::column_not_found};
                }
                return ss.str();
            }
        };
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct statement_serializer<as_optional_t<T>, void> {
            using statement_type = as_optional_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                return serialize(statement.value, context);
            }
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct statement_serializer<std::reference_wrapper<T>, void> {
            using statement_type = std::reference_wrapper<T>;

            template<class Ctx>
            std::string operator()(const statement_type& s, const Ctx& context) const {
                return serialize(s.get(), context);
            }
        };

        template<class T>
        struct statement_serializer<alias_holder<T>, void> {
            using statement_type = alias_holder<T>;

            template<class Ctx>
            std::string operator()(const statement_type&, const Ctx&) {
                return T::get();
            }
        };

        template<class... TargetArgs, class... ActionsArgs>
        struct statement_serializer<upsert_clause<std::tuple<TargetArgs...>, std::tuple<ActionsArgs...>>, void> {
            using statement_type = upsert_clause<std::tuple<TargetArgs...>, std::tuple<ActionsArgs...>>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "ON CONFLICT";
                iterate_tuple(statement.target_args, [&ss, &context](auto& value) {
                    using value_type = typename std::decay<decltype(value)>::type;
                    auto needParenthesis = std::is_member_pointer<value_type>::value;
                    ss << ' ';
                    if(needParenthesis) {
                        ss << '(';
                    }
                    ss << serialize(value, context);
                    if(needParenthesis) {
                        ss << ')';
                    }
                });
                ss << ' ' << "DO";
                if(std::tuple_size<typename statement_type::actions_tuple>::value == 0) {
                    ss << " NOTHING";
                } else {
                    ss << " UPDATE";
                    auto updateContext = context;
                    updateContext.use_parentheses = false;
                    iterate_tuple(statement.actions, [&ss, &updateContext](auto& value) {
                        ss << ' ' << serialize(value, updateContext);
                    });
                }
                return ss.str();
            }
        };

        template<class R, class S, class... Args>
        struct statement_serializer<built_in_function_t<R, S, Args...>, void> {
            using statement_type = built_in_function_t<R, S, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                if(context.use_parentheses) {
                    ss << '(';
                }
                ss << statement.serialize() << "(";
                std::vector<std::string> args;
                using args_type = typename std::decay<decltype(statement)>::type::args_type;
                args.reserve(std::tuple_size<args_type>::value);
                iterate_tuple(statement.args, [&args, &context](auto& v) {
                    args.push_back(serialize(v, context));
                });
                for(size_t i = 0; i < args.size(); ++i) {
                    ss << args[i];
                    if(i < args.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")";
                if(context.use_parentheses) {
                    ss << ')';
                }
                return ss.str();
            }
        };

        template<class R, class S, class... Args>
        struct statement_serializer<built_in_aggregate_function_t<R, S, Args...>, void>
            : statement_serializer<built_in_function_t<R, S, Args...>, void> {};

        template<class F, class... Args>
        struct statement_serializer<function_call<F, Args...>, void> {
            using statement_type = function_call<F, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using args_tuple = std::tuple<Args...>;

                std::stringstream ss;
                ss << F::name() << "(";
                auto index = 0;
                iterate_tuple(statement.args, [&context, &ss, &index](auto& v) {
                    auto value = serialize(v, context);
                    ss << value;
                    if(index < std::tuple_size<args_tuple>::value - 1) {
                        ss << ", ";
                    }
                    ++index;
                });
                ss << ")";
                return ss.str();
            }
        };

        template<class T, class E>
        struct statement_serializer<as_t<T, E>, void> {
            using statement_type = as_t<T, E>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                auto tableAliasString = alias_extractor<T>::get();
                return serialize(c.expression, context) + " AS " + tableAliasString;
            }
        };

        template<class T, class P>
        struct statement_serializer<alias_column_t<T, P>, void> {
            using statement_type = alias_column_t<T, P>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                if(!context.skip_table_name) {
                    ss << "'" << T::get() << "'.";
                }
                auto newContext = context;
                newContext.skip_table_name = true;
                ss << serialize(c.column, newContext);
                return ss.str();
            }
        };

        template<class O, class F>
        struct statement_serializer<F O::*, void> {
            using statement_type = F O::*;

            template<class Ctx>
            std::string operator()(const statement_type& m, const Ctx& context) const {
                std::stringstream ss;
                if(!context.skip_table_name) {
                    ss << "'" << context.impl.find_table_name(typeid(O)) << "'.";
                }
                if(auto columnnamePointer = find_column_name(context.impl, m)) {
                    ss << "\"" << *columnnamePointer << "\"";
                } else {
                    throw std::system_error{orm_error_code::column_not_found};
                }
                return ss.str();
            }
        };

        template<>
        struct statement_serializer<rowid_t, void> {
            using statement_type = rowid_t;

            template<class Ctx>
            std::string operator()(const statement_type& s, const Ctx&) const {
                return static_cast<std::string>(s);
            }
        };

        template<>
        struct statement_serializer<oid_t, void> {
            using statement_type = oid_t;

            template<class Ctx>
            std::string operator()(const statement_type& s, const Ctx&) const {
                return static_cast<std::string>(s);
            }
        };

        template<>
        struct statement_serializer<_rowid_t, void> {
            using statement_type = _rowid_t;

            template<class Ctx>
            std::string operator()(const statement_type& s, const Ctx&) const {
                return static_cast<std::string>(s);
            }
        };

        template<class O>
        struct statement_serializer<table_rowid_t<O>, void> {
            using statement_type = table_rowid_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& s, const Ctx& context) const {
                std::stringstream ss;
                if(!context.skip_table_name) {
                    ss << "'" << context.impl.find_table_name(typeid(O)) << "'.";
                }
                ss << static_cast<std::string>(s);
                return ss.str();
            }
        };

        template<class O>
        struct statement_serializer<table_oid_t<O>, void> {
            using statement_type = table_oid_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& s, const Ctx& context) const {
                std::stringstream ss;
                if(!context.skip_table_name) {
                    ss << "'" << context.impl.find_table_name(typeid(O)) << "'.";
                }
                ss << static_cast<std::string>(s);
                return ss.str();
            }
        };

        template<class O>
        struct statement_serializer<table__rowid_t<O>, void> {
            using statement_type = table__rowid_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& s, const Ctx& context) const {
                std::stringstream ss;
                if(!context.skip_table_name) {
                    ss << "'" << context.impl.find_table_name(typeid(O)) << "'.";
                }
                ss << static_cast<std::string>(s);
                return ss.str();
            }
        };

        template<class L, class R, class... Ds>
        struct statement_serializer<binary_operator<L, R, Ds...>, void> {
            using statement_type = binary_operator<L, R, Ds...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                auto lhs = serialize(statement.lhs, context);
                auto rhs = serialize(statement.rhs, context);
                std::stringstream ss;
                if(context.use_parentheses) {
                    ss << '(';
                }
                ss << lhs << " " << statement.serialize() << " " << rhs;
                if(context.use_parentheses) {
                    ss << ')';
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<count_asterisk_t<T>, void> {
            using statement_type = count_asterisk_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type&, const Ctx& context) const {
                return serialize(count_asterisk_without_type{}, context);
            }
        };

        template<>
        struct statement_serializer<count_asterisk_without_type, void> {
            using statement_type = count_asterisk_without_type;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx&) const {
                std::stringstream ss;
                auto functionName = c.serialize();
                ss << functionName << "(*)";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<distinct_t<T>, void> {
            using statement_type = distinct_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                auto expr = serialize(c.value, context);
                ss << static_cast<std::string>(c) << "(" << expr << ")";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<all_t<T>, void> {
            using statement_type = all_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                auto expr = serialize(c.value, context);
                ss << static_cast<std::string>(c) << "(" << expr << ")";
                return ss.str();
            }
        };

        template<class T, class F>
        struct statement_serializer<column_pointer<T, F>, void> {
            using statement_type = column_pointer<T, F>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                if(!context.skip_table_name) {
                    ss << "'" << context.impl.find_table_name(typeid(T)) << "'.";
                }
                if(auto columnNamePointer = find_column_name(context.impl, c)) {
                    ss << "\"" << *columnNamePointer << "\"";
                } else {
                    throw std::system_error{orm_error_code::column_not_found};
                }
                return ss.str();
            }
        };

        template<class T, class E>
        struct statement_serializer<cast_t<T, E>, void> {
            using statement_type = cast_t<T, E>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << " (";
                ss << serialize(c.expression, context) << " AS " << type_printer<T>().print() << ")";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<T,
                                    typename std::enable_if<is_base_of_template<T, compound_operator>::value>::type> {
            using statement_type = T;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << serialize(c.left, context) << " ";
                ss << static_cast<std::string>(c) << " ";
                ss << serialize(c.right, context);
                return ss.str();
            }
        };

        template<class R, class T, class E, class... Args>
        struct statement_serializer<simple_case_t<R, T, E, Args...>, void> {
            using statement_type = simple_case_t<R, T, E, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << "CASE ";
                c.case_expression.apply([&ss, context](auto& c_) {
                    ss << serialize(c_, context) << " ";
                });
                iterate_tuple(c.args, [&ss, context](auto& pair) {
                    ss << "WHEN " << serialize(pair.first, context) << " ";
                    ss << "THEN " << serialize(pair.second, context) << " ";
                });
                c.else_expression.apply([&ss, context](auto& el) {
                    ss << "ELSE " << serialize(el, context) << " ";
                });
                ss << "END";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<is_null_t<T>, void> {
            using statement_type = is_null_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << serialize(c.t, context) << " " << static_cast<std::string>(c);
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<is_not_null_t<T>, void> {
            using statement_type = is_not_null_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << serialize(c.t, context) << " " << static_cast<std::string>(c);
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<bitwise_not_t<T>, void> {
            using statement_type = bitwise_not_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << statement.serialize() << " ";
                auto cString = serialize(statement.argument, context);
                ss << " (" << cString << " )";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<negated_condition_t<T>, void> {
            using statement_type = negated_condition_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << " ";
                auto cString = serialize(c.c, context);
                ss << " (" << cString << " )";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<T, typename std::enable_if<is_base_of_template<T, binary_condition>::value>::type> {
            using statement_type = T;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                auto leftString = serialize(c.l, context);
                auto rightString = serialize(c.r, context);
                std::stringstream ss;
                if(context.use_parentheses) {
                    ss << "(";
                }
                ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
                if(context.use_parentheses) {
                    ss << ")";
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<named_collate<T>, void> {
            using statement_type = named_collate<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                auto newContext = context;
                newContext.use_parentheses = false;
                auto res = serialize(c.expr, newContext);
                return res + " " + static_cast<std::string>(c);
            }
        };

        template<class T>
        struct statement_serializer<collate_t<T>, void> {
            using statement_type = collate_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                auto newContext = context;
                newContext.use_parentheses = false;
                auto res = serialize(c.expr, newContext);
                return res + " " + static_cast<std::string>(c);
            }
        };

        template<class L, class A>
        struct statement_serializer<dynamic_in_t<L, A>, void> {
            using statement_type = dynamic_in_t<L, A>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto leftString = serialize(statement.left, context);
                ss << leftString << " ";
                if(!statement.negative) {
                    ss << "IN";
                } else {
                    ss << "NOT IN";
                }
                ss << " ";
                constexpr auto isCompoundOperator = is_base_of_template<A, compound_operator>::value;
                if(isCompoundOperator) {
                    ss << '(';
                }
                auto newContext = context;
                newContext.use_parentheses = true;
                ss << serialize(statement.argument, newContext);
                if(isCompoundOperator) {
                    ss << ')';
                }
                return ss.str();
            }
        };

        template<class L, class E>
        struct statement_serializer<dynamic_in_t<L, std::vector<E>>, void> {
            using statement_type = dynamic_in_t<L, std::vector<E>>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto leftString = serialize(statement.left, context);
                ss << leftString << " ";
                if(!statement.negative) {
                    ss << "IN";
                } else {
                    ss << "NOT IN";
                }
                ss << " (";
                for(size_t index = 0; index < statement.argument.size(); ++index) {
                    auto& value = statement.argument[index];
                    ss << serialize(value, context);
                    if(index < statement.argument.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")";
                return ss.str();
            }
        };

        template<class L, class... Args>
        struct statement_serializer<in_t<L, Args...>, void> {
            using statement_type = in_t<L, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto leftString = serialize(statement.left, context);
                ss << leftString << " ";
                if(!statement.negative) {
                    ss << "IN";
                } else {
                    ss << "NOT IN";
                }
                ss << " ";
                std::vector<std::string> args;
                using args_type = std::tuple<Args...>;
                args.reserve(std::tuple_size<args_type>::value);
                const auto theOnlySelect = std::tuple_size<args_type>::value == 1 &&
                                           is_select<typename std::tuple_element<0, args_type>::type>::value;
                if(!theOnlySelect) {
                    ss << "(";
                }
                iterate_tuple(statement.argument, [&args, &context](auto& v) {
                    args.push_back(serialize(v, context));
                });
                for(size_t i = 0; i < args.size(); ++i) {
                    ss << args[i];
                    if(i < args.size() - 1) {
                        ss << ", ";
                    }
                }
                if(!theOnlySelect) {
                    ss << ")";
                }
                return ss.str();
            }
        };

        template<class A, class T, class E>
        struct statement_serializer<like_t<A, T, E>, void> {
            using statement_type = like_t<A, T, E>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << serialize(c.arg, context) << " ";
                ss << static_cast<std::string>(c) << " ";
                ss << serialize(c.pattern, context);
                c.arg3.apply([&ss, &context](auto& value) {
                    ss << " ESCAPE " << serialize(value, context);
                });
                return ss.str();
            }
        };

        template<class A, class T>
        struct statement_serializer<glob_t<A, T>, void> {
            using statement_type = glob_t<A, T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << serialize(c.arg, context) << " ";
                ss << static_cast<std::string>(c) << " ";
                ss << serialize(c.pattern, context);
                return ss.str();
            }
        };

        template<class A, class T>
        struct statement_serializer<between_t<A, T>, void> {
            using statement_type = between_t<A, T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                auto expr = serialize(c.expr, context);
                ss << expr << " " << static_cast<std::string>(c) << " ";
                ss << serialize(c.b1, context);
                ss << " AND ";
                ss << serialize(c.b2, context);
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<exists_t<T>, void> {
            using statement_type = exists_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "EXISTS ";
                ss << serialize(statement.expression, context);
                return ss.str();
            }
        };

        template<>
        struct statement_serializer<autoincrement_t, void> {
            using statement_type = autoincrement_t;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx&) const {
                return "AUTOINCREMENT";
            }
        };

        template<class... Cs>
        struct statement_serializer<primary_key_t<Cs...>, void> {
            using statement_type = primary_key_t<Cs...>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                auto res = static_cast<std::string>(c);
                using columns_tuple = typename statement_type::columns_tuple;
                auto columnsCount = std::tuple_size<columns_tuple>::value;
                if(columnsCount) {
                    res += "(";
                    decltype(columnsCount) columnIndex = 0;
                    iterate_tuple(c.columns, [&context, &res, &columnIndex, columnsCount](auto& column) {
                        if(auto columnNamePointer = find_column_name(context.impl, column)) {
                            res += *columnNamePointer;
                            if(columnIndex < columnsCount - 1) {
                                res += ", ";
                            }
                            ++columnIndex;
                        } else {
                            throw std::system_error{orm_error_code::column_not_found};
                        }
                    });
                    res += ")";
                }
                return res;
            }
        };

        template<class... Args>
        struct statement_serializer<unique_t<Args...>, void> {
            using statement_type = unique_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                auto res = static_cast<std::string>(c);
                using columns_tuple = typename statement_type::columns_tuple;
                auto columnsCount = std::tuple_size<columns_tuple>::value;
                if(columnsCount) {
                    res += "(";
                    decltype(columnsCount) columnIndex = 0;
                    iterate_tuple(c.columns, [&context, &res, &columnIndex, columnsCount](auto& column) {
                        if(auto columnNamePointer = find_column_name(context.impl, column)) {
                            res += *columnNamePointer;
                            if(columnIndex < columnsCount - 1) {
                                res += ", ";
                            }
                            ++columnIndex;
                        } else {
                            throw std::system_error{orm_error_code::column_not_found};
                        }
                    });
                    res += ")";
                }
                return res;
            }
        };

        template<>
        struct statement_serializer<collate_constraint_t, void> {
            using statement_type = collate_constraint_t;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx&) const {
                return static_cast<std::string>(c);
            }
        };

        template<class T>
        struct statement_serializer<default_t<T>, void> {
            using statement_type = default_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                return static_cast<std::string>(c) + " (" + serialize(c.value, context) + ")";
            }
        };

        template<class... Cs, class... Rs>
        struct statement_serializer<foreign_key_t<std::tuple<Cs...>, std::tuple<Rs...>>, void> {
            using statement_type = foreign_key_t<std::tuple<Cs...>, std::tuple<Rs...>>;

            template<class Ctx>
            std::string operator()(const statement_type& fk, const Ctx& context) const {
                std::stringstream ss;
                std::vector<std::string> columnNames;
                using columns_type_t = typename std::decay<decltype(fk)>::type::columns_type;
                constexpr const size_t columnsCount = std::tuple_size<columns_type_t>::value;
                columnNames.reserve(columnsCount);
                iterate_tuple(fk.columns, [&columnNames, &context](auto& v) {
                    if(auto columnNamePointer = find_column_name(context.impl, v)) {
                        columnNames.push_back(*columnNamePointer);
                    } else {
                        columnNames.push_back({});
                    }
                });
                ss << "FOREIGN KEY(";
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << "'" << columnNames[i] << "'";
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
                    ss << '\'' << refTableName << '\'';
                }
                iterate_tuple(fk.references, [&referencesNames, &context](auto& v) {
                    if(auto columnNamePointer = find_column_name(context.impl, v)) {
                        referencesNames.push_back(*columnNamePointer);
                    } else {
                        referencesNames.push_back({});
                    }
                });
                ss << "(";
                for(size_t i = 0; i < referencesNames.size(); ++i) {
                    ss << "'" << referencesNames[i] << "'";
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
        struct statement_serializer<check_t<T>, void> {
            using statement_type = check_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                return "CHECK (" + serialize(statement.expression, context) + ")";
            }
        };
#if SQLITE_VERSION_NUMBER >= 3031000
        template<class T>
        struct statement_serializer<generated_always_t<T>, void> {
            using statement_type = generated_always_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                if(statement.full) {
                    ss << "GENERATED ALWAYS ";
                }
                ss << "AS (";
                ss << serialize(statement.expression, context) << ")";
                switch(statement.storage) {
                    case decltype(statement.storage)::not_specified:
                        //..
                        break;
                    case decltype(statement.storage)::virtual_:
                        ss << " VIRTUAL";
                        break;
                    case decltype(statement.storage)::stored:
                        ss << " STORED";
                        break;
                }
                return ss.str();
            }
        };
#endif
        template<class O, class T, class G, class S, class... Op>
        struct statement_serializer<column_t<O, T, G, S, Op...>, void> {
            using statement_type = column_t<O, T, G, S, Op...>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
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
                        [&constraintsStrings, &primaryKeyIndex, &autoincrementIndex, &tupleIndex, &context](auto& v) {
                            using constraint_type = typename std::decay<decltype(v)>::type;
                            constraintsStrings.push_back(serialize(v, context));
                            if(is_primary_key<constraint_type>::value) {
                                primaryKeyIndex = tupleIndex;
                            } else if(std::is_same<autoincrement_t, constraint_type>::value) {
                                autoincrementIndex = tupleIndex;
                            }
                            ++tupleIndex;
                        });
                    if(primaryKeyIndex != -1 && autoincrementIndex != -1 && autoincrementIndex < primaryKeyIndex) {
                        iter_swap(constraintsStrings.begin() + primaryKeyIndex,
                                  constraintsStrings.begin() + autoincrementIndex);
                    }
                    for(auto& str: constraintsStrings) {
                        ss << str << ' ';
                    }
                }
                if(c.not_null()) {
                    ss << "NOT NULL ";
                }
                return ss.str();
            }
        };

        template<class T, class... Args>
        struct statement_serializer<remove_all_t<T, Args...>, void> {
            using statement_type = remove_all_t<T, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& rem, const Ctx& context) const {
                auto& tImpl = pick_impl<T>(context.impl);
                std::stringstream ss;
                ss << "DELETE FROM '" << tImpl.table.name << "' ";
                iterate_tuple(rem.conditions, [&context, &ss](auto& v) {
                    ss << serialize(v, context);
                });
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<replace_t<T>, void> {
            using statement_type = replace_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using expression_type = typename std::decay<decltype(statement)>::type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& tImpl = pick_impl<object_type>(context.impl);
                std::stringstream ss;
                ss << "REPLACE INTO '" << tImpl.table.name << "' (";

                auto columnIndex = 0;
                auto columnsCount = tImpl.table.non_generated_columns_count();
                tImpl.table.for_each_column([&ss, &columnIndex, columnsCount](auto& column) {
                    if(column.is_generated()) {
                        return;
                    }
                    ss << "\"" << column.name << "\"";
                    if(columnIndex < columnsCount - 1) {
                        ss << ", ";
                    } else {
                        ss << ")";
                    }
                    ++columnIndex;
                });
                ss << " VALUES (";
                columnIndex = 0;
                auto& object = get_ref(statement.object);
                tImpl.table.for_each_column([&ss, &columnIndex, columnsCount, &object, &context](auto& column) {
                    if(column.is_generated()) {
                        return;
                    }
                    if(column.member_pointer) {
                        ss << serialize(object.*column.member_pointer, context);
                    } else {
                        ss << serialize((object.*column.getter)(), context);
                    }
                    if(columnIndex < columnsCount - 1) {
                        ss << ", ";
                    } else {
                        ss << ")";
                    }
                    ++columnIndex;
                });
                return ss.str();
            }
        };

        template<class T, class... Cols>
        struct statement_serializer<insert_explicit<T, Cols...>, void> {
            using statement_type = insert_explicit<T, Cols...>;

            template<class Ctx>
            std::string operator()(const statement_type& ins, const Ctx& context) const {
                constexpr const size_t colsCount = std::tuple_size<std::tuple<Cols...>>::value;
                static_assert(colsCount > 0, "Use insert or replace with 1 argument instead");
                using expression_type = typename std::decay<decltype(ins)>::type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& tImpl = pick_impl<object_type>(context.impl);
                std::stringstream ss;
                ss << "INSERT INTO '" << tImpl.table.name << "' ";
                ss << "(";
                {
                    auto columnsContext = context;
                    columnsContext.skip_table_name = true;
                    auto index = 0;
                    iterate_tuple(ins.columns.columns, [&ss, &columnsContext, &index](auto& memberPointer) {
                        constexpr const size_t colsCount = std::tuple_size<std::tuple<Cols...>>::value;
                        auto columnName = serialize(memberPointer, columnsContext);
                        if(!columnName.empty()) {
                            ss << columnName;
                            if(index < colsCount - 1) {
                                ss << ",";
                            } else {
                                ss << ")";
                            }
                            ss << " ";
                            ++index;
                        } else {
                            throw std::system_error{orm_error_code::column_not_found};
                        }
                    });
                }
                ss << "VALUES (";
                auto index = 0;
                auto& object = get_ref(ins.obj);
                iterate_tuple(ins.columns.columns, [&ss, &context, &index, &object](auto& memberPointer) {
                    constexpr const size_t colsCount = std::tuple_size<std::tuple<Cols...>>::value;
                    using member_pointer_type = typename std::decay<decltype(memberPointer)>::type;
                    static_assert(!is_setter<member_pointer_type>::value,
                                  "Unable to use setter within insert explicit");

                    std::string valueString;
                    static_if<is_getter<member_pointer_type>{}>(
                        [&valueString, &memberPointer, &context](auto& object) {
                            valueString = serialize((object.*memberPointer)(), context);
                        },
                        [&valueString, &memberPointer, &context](auto& object) {
                            valueString = serialize(object.*memberPointer, context);
                        })(object);
                    ss << valueString;
                    if(index < colsCount - 1) {
                        ss << ", ";
                    } else {
                        ss << ")";
                    }
                    ++index;
                });
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<update_t<T>, void> {
            using statement_type = update_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using expression_type = typename std::decay<decltype(statement)>::type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& tImpl = pick_impl<object_type>(context.impl);

                std::stringstream ss;
                ss << "UPDATE '" << tImpl.table.name << "' SET";
                //                std::vector<std::string> setColumnNames;
                auto columnIndex = 0;
                auto& object = get_ref(statement.object);
                tImpl.table.for_each_column([&tImpl, &columnIndex, &ss, &object, &context](auto& column) {
                    if(!column.template has<primary_key_t<>>() &&
                       !tImpl.table.exists_in_composite_primary_key(column)) {
                        if(0 == columnIndex) {
                            ss << ' ';
                        } else {
                            ss << ", ";
                        }
                        ss << "\"" << column.name << "\" = ";
                        if(column.member_pointer) {
                            ss << serialize(object.*column.member_pointer, context);
                        } else {
                            ss << serialize((object.*column.getter)(), context);
                        }
                        ++columnIndex;
                    }
                });
                ss << " WHERE";
                columnIndex = 0;
                tImpl.table.for_each_column([&tImpl, &columnIndex, &ss, &object, &context](auto& column) {
                    if(column.template has<primary_key_t<>>() || tImpl.table.exists_in_composite_primary_key(column)) {
                        if(0 == columnIndex) {
                            ss << ' ';
                        } else {
                            ss << " AND ";
                        }
                        ss << "\"" << column.name << "\" = ";
                        if(column.member_pointer) {
                            ss << serialize(object.*column.member_pointer, context);
                        } else {
                            ss << serialize((object.*column.getter)(), context);
                        }
                        ++columnIndex;
                    }
                });
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<set_t<Args...>, void> {
            using statement_type = set_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "SET";
                auto assignsCount = std::tuple_size<typename statement_type::assigns_type>::value;
                decltype(assignsCount) assignIndex = 0;
                auto leftContext = context;
                leftContext.skip_table_name = true;
                iterate_tuple(statement.assigns,
                              [&ss, &context, &leftContext, &assignIndex, assignsCount](auto& value) {
                                  ss << ' ' << serialize(value.lhs, leftContext);
                                  ss << ' ' << value.serialize() << ' ';
                                  ss << serialize(value.rhs, context);
                                  if(assignIndex < assignsCount - 1) {
                                      ss << ",";
                                  }
                                  ++assignIndex;
                              });
                return ss.str();
            }
        };

        template<class... Args, class... Wargs>
        struct statement_serializer<update_all_t<set_t<Args...>, Wargs...>, void> {
            using statement_type = update_all_t<set_t<Args...>, Wargs...>;

            template<class Ctx>
            std::string operator()(const statement_type& upd, const Ctx& context) const {
                std::stringstream ss;
                ss << "UPDATE";
                table_name_collector collector([&context](std::type_index ti) {
                    return context.impl.find_table_name(ti);
                });
                iterate_ast(upd.set.assigns, collector);
                if(!collector.table_names.empty()) {
                    if(collector.table_names.size() >= 1) {
                        ss << " '" << collector.table_names.begin()->first << "' SET";
                        std::vector<std::string> setPairs;
                        auto leftContext = context;
                        leftContext.skip_table_name = true;
                        iterate_tuple(upd.set.assigns, [&context, &leftContext, &setPairs](auto& asgn) {
                            std::stringstream sss;
                            sss << serialize(asgn.lhs, leftContext);
                            sss << ' ' << asgn.serialize() << ' ';
                            sss << serialize(asgn.rhs, context);
                            setPairs.push_back(sss.str());
                        });
                        auto setPairsCount = setPairs.size();
                        for(size_t i = 0; i < setPairsCount; ++i) {
                            ss << ' ' << setPairs[i];
                            if(i < setPairsCount - 1) {
                                ss << ",";
                            }
                        }
                        iterate_tuple(upd.conditions, [&context, &ss](auto& v) {
                            ss << ' ' << serialize(v, context);
                        });
                        return ss.str();
                    } else {
                        throw std::system_error{orm_error_code::no_tables_specified};
                    }
                } else {
                    throw std::system_error{orm_error_code::incorrect_set_fields_specified};
                }
            }
        };

        template<class T>
        struct statement_serializer<insert_t<T>, void> {
            using statement_type = insert_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using object_type = typename expression_object_type<statement_type>::type;
                auto& tImpl = pick_impl<object_type>(context.impl);

                std::stringstream ss;
                ss << "INSERT INTO '" << tImpl.table.name << "' ";

                auto columnIndex = 0;
                tImpl.table.for_each_column([&tImpl, &columnIndex, &ss](auto& column) {
                    using table_type = typename std::decay<decltype(tImpl.table)>::type;
                    if(table_type::is_without_rowid ||
                       (!column.template has<primary_key_t<>>() &&
                        !tImpl.table.exists_in_composite_primary_key(column) && !column.is_generated())) {
                        if(0 == columnIndex) {
                            ss << '(';
                        } else {
                            ss << ", ";
                        }
                        ss << "\"" << column.name << "\"";
                        ++columnIndex;
                    }
                });
                auto columnsToInsertCount = columnIndex;
                if(columnsToInsertCount > 0) {
                    ss << ')';
                } else {
                    ss << "DEFAULT";
                }
                ss << " VALUES ";
                if(columnsToInsertCount) {
                    ss << "(";
                    columnIndex = 0;
                    auto& object = get_ref(statement.object);
                    tImpl.table.for_each_column(
                        [&tImpl, &columnIndex, &ss, columnsToInsertCount, &context, &object](auto& column) {
                            using table_type = typename std::decay<decltype(tImpl.table)>::type;
                            if(table_type::is_without_rowid ||
                               (!column.template has<primary_key_t<>>() &&
                                !tImpl.table.exists_in_composite_primary_key(column) && !column.is_generated())) {
                                if(column.member_pointer) {
                                    ss << serialize(object.*column.member_pointer, context);
                                } else {
                                    ss << serialize((object.*column.getter)(), context);
                                }
                                if(columnIndex < columnsToInsertCount - 1) {
                                    ss << ", ";
                                }
                                ++columnIndex;
                            }
                        });
                    ss << ")";
                }

                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<into_t<T>, void> {
            using statement_type = into_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto& tImpl = pick_impl<T>(context.impl);
                ss << "INTO " << tImpl.table.name;
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<columns_t<Args...>, void> {
            using statement_type = columns_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto index = 0;
                if(context.use_parentheses) {
                    ss << '(';
                }
                iterate_tuple(statement.columns, [&context, &ss, &index](auto& value) {
                    ss << serialize(value, context);
                    if(index < int(std::tuple_size<std::tuple<Args...>>::value) - 1) {
                        ss << ", ";
                    }
                    ++index;
                });
                if(context.use_parentheses) {
                    ss << ')';
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<
            T,
            typename std::enable_if<is_insert_raw<T>::value || is_replace_raw<T>::value>::type> {
            using statement_type = T;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                if(is_insert_raw<T>::value) {
                    ss << "INSERT";
                } else {
                    ss << "REPLACE";
                }
                iterate_tuple(statement.args, [&context, &ss](auto& value) {
                    using value_type = typename std::decay<decltype(value)>::type;
                    ss << ' ';
                    if(is_columns<value_type>::value) {
                        auto newContext = context;
                        newContext.skip_table_name = true;
                        newContext.use_parentheses = true;
                        ss << serialize(value, newContext);
                    } else if(is_values<value_type>::value || is_select<value_type>::value) {
                        auto newContext = context;
                        newContext.use_parentheses = false;
                        ss << serialize(value, newContext);
                    } else {
                        ss << serialize(value, context);
                    }
                });
                return ss.str();
            }
        };

        template<class T, class... Ids>
        struct statement_serializer<remove_t<T, Ids...>, void> {
            using statement_type = remove_t<T, Ids...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                auto& tImpl = pick_impl<T>(context.impl);
                std::stringstream ss;
                ss << "DELETE FROM '" << tImpl.table.name << "' ";
                ss << "WHERE";
                std::vector<std::string> idsStrings;
                idsStrings.reserve(std::tuple_size<typename statement_type::ids_type>::value);
                iterate_tuple(statement.ids, [&idsStrings, &context](auto& idValue) {
                    idsStrings.push_back(serialize(idValue, context));
                });
                auto index = 0;
                tImpl.table.for_each_primary_key_column([&ss, &index, &tImpl, &idsStrings](auto& memberPointer) {
                    if(index > 0) {
                        ss << " AND";
                    }
                    if(auto* columnNamePointer = tImpl.table.find_column_name(memberPointer)) {
                        ss << " \"" << *columnNamePointer << "\""
                           << " = " << idsStrings[index];
                    } else {
                        throw std::system_error{orm_error_code::column_not_found};
                    }
                    ++index;
                });
                return ss.str();
            }
        };

        template<class It, class L, class O>
        struct statement_serializer<replace_range_t<It, L, O>, void> {
            using statement_type = replace_range_t<It, L, O>;

            template<class Ctx>
            std::string operator()(const statement_type& rep, const Ctx& context) const {
                auto valuesCount = static_cast<int>(std::distance(rep.range.first, rep.range.second));
                using expression_type = typename std::decay<decltype(rep)>::type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& tImpl = pick_impl<object_type>(context.impl);
                std::stringstream ss;
                ss << "REPLACE INTO '" << tImpl.table.name << "' (";

                auto columnIndex = 0;
                auto columnsCount = tImpl.table.non_generated_columns_count();
                tImpl.table.for_each_column([&ss, &columnIndex, columnsCount](auto& column) {
                    if(column.is_generated()) {
                        return;
                    }
                    ss << " \"" << column.name << "\"";
                    if(columnIndex < columnsCount - 1) {
                        ss << ",";
                    } else {
                        ss << ")";
                    }
                    ++columnIndex;
                });
                ss << " VALUES ";
                auto valuesString = [columnsCount] {
                    std::stringstream ss_;
                    ss_ << "(";
                    for(auto i = 0; i < columnsCount; ++i) {
                        ss_ << "?";
                        if(i < columnsCount - 1) {
                            ss_ << ", ";
                        } else {
                            ss_ << ")";
                        }
                    }
                    return ss_.str();
                }();
                for(auto i = 0; i < valuesCount; ++i) {
                    ss << valuesString;
                    if(i < valuesCount - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                return ss.str();
            }
        };

        template<class It, class L, class O>
        struct statement_serializer<insert_range_t<It, L, O>, void> {
            using statement_type = insert_range_t<It, L, O>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                const auto valuesCount = static_cast<int>(std::distance(statement.range.first, statement.range.second));
                using object_type = typename expression_object_type<statement_type>::type;
                auto& tImpl = pick_impl<object_type>(context.impl);

                std::stringstream ss;
                ss << "INSERT INTO '" << tImpl.table.name << "' ";
                std::vector<std::string> columnNames;

                tImpl.table.for_each_column([&columnNames, &tImpl](auto& column) {
                    using table_type = typename std::decay<decltype(tImpl.table)>::type;
                    if(table_type::is_without_rowid ||
                       (!column.template has<primary_key_t<>>() &&
                        !tImpl.table.exists_in_composite_primary_key(column) && !column.is_generated())) {
                        columnNames.emplace_back(column.name);
                    }
                });

                const auto columnNamesCount = columnNames.size();
                if(columnNamesCount) {
                    ss << "(";
                    for(size_t i = 0; i < columnNamesCount; ++i) {
                        ss << "\"" << columnNames[i] << "\"";
                        if(i < columnNamesCount - 1) {
                            ss << ",";
                        } else {
                            ss << ")";
                        }
                        ss << " ";
                    }
                } else {
                    ss << "DEFAULT ";
                }
                ss << "VALUES ";
                if(columnNamesCount) {
                    auto valuesString = [columnNamesCount] {
                        std::stringstream ss_;
                        ss_ << "(";
                        for(size_t i = 0; i < columnNamesCount; ++i) {
                            ss_ << "?";
                            if(i < columnNamesCount - 1) {
                                ss_ << ", ";
                            } else {
                                ss_ << ")";
                            }
                        }
                        return ss_.str();
                    }();
                    for(auto i = 0; i < valuesCount; ++i) {
                        ss << valuesString;
                        if(i < valuesCount - 1) {
                            ss << ",";
                        }
                        ss << " ";
                    }
                } else if(valuesCount != 1) {
                    throw std::system_error{orm_error_code::cannot_use_default_value};
                }
                return ss.str();
            }
        };

        template<class T, class Ctx>
        std::string serialize_get_all_impl(const T& get, const Ctx& context) {
            using primary_type = typename T::type;

            table_name_collector collector;
            collector.table_names.insert(
                std::make_pair(context.impl.find_table_name(typeid(primary_type)), std::string{}));
            iterate_ast(get.conditions, collector);
            std::stringstream ss;
            ss << "SELECT";
            auto& tImpl = pick_impl<primary_type>(context.impl);
            auto columnIndex = 0;
            auto columnsCount = tImpl.table.count_columns_amount();
            tImpl.table.for_each_column([&ss, &columnIndex, columnsCount, &tImpl](auto& column) {
                ss << " \"" << tImpl.table.name << "\"."
                   << "\"" << column.name << "\"";
                if(columnIndex < columnsCount - 1) {
                    ss << ",";
                }
                ++columnIndex;
            });
            ss << " FROM ";
            std::vector<std::pair<std::string, std::string>> tableNames(collector.table_names.begin(),
                                                                        collector.table_names.end());
            for(size_t i = 0; i < tableNames.size(); ++i) {
                auto& tableNamePair = tableNames[i];
                ss << "'" << tableNamePair.first << "' ";
                if(!tableNamePair.second.empty()) {
                    ss << tableNamePair.second << " ";
                }
                if(int(i) < int(tableNames.size()) - 1) {
                    ss << ",";
                }
                ss << " ";
            }
            iterate_tuple(get.conditions, [&context, &ss](auto& v) {
                ss << serialize(v, context);
            });
            return ss.str();
        }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class R, class... Args>
        struct statement_serializer<get_all_optional_t<T, R, Args...>, void> {
            using statement_type = get_all_optional_t<T, R, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& get, const Ctx& context) const {
                return serialize_get_all_impl(get, context);
            }
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class T, class R, class... Args>
        struct statement_serializer<get_all_pointer_t<T, R, Args...>, void> {
            using statement_type = get_all_pointer_t<T, R, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& get, const Ctx& context) const {
                return serialize_get_all_impl(get, context);
            }
        };

        template<class T, class R, class... Args>
        struct statement_serializer<get_all_t<T, R, Args...>, void> {
            using statement_type = get_all_t<T, R, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& get, const Ctx& context) const {
                return serialize_get_all_impl(get, context);
            }
        };

        template<class T, class Ctx>
        std::string serialize_get_impl(const T&, const Ctx& context) {
            using primary_type = typename T::type;
            auto& tImpl = pick_impl<primary_type>(context.impl);
            std::stringstream ss;
            ss << "SELECT";
            auto columnIndex = 0;
            auto columnsCount = tImpl.table.count_columns_amount();
            tImpl.table.for_each_column([&ss, &columnIndex, columnsCount](auto& column) {
                ss << " \"" << column.name << "\"";
                if(columnIndex < columnsCount - 1) {
                    ss << ", ";
                }
                ++columnIndex;
            });
            ss << " FROM '" << tImpl.table.name << "' WHERE ";
            auto primaryKeyColumnNames = tImpl.table.primary_key_column_names();
            if(!primaryKeyColumnNames.empty()) {
                for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << " \"" << primaryKeyColumnNames[i] << "\""
                       << " = ?";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << " AND";
                    }
                }
                return ss.str();
            } else {
                throw std::system_error{orm_error_code::table_has_no_primary_key_column};
            }
        }

        template<class T, class... Ids>
        struct statement_serializer<get_t<T, Ids...>, void> {
            using statement_type = get_t<T, Ids...>;

            template<class Ctx>
            std::string operator()(const statement_type& get, const Ctx& context) const {
                return serialize_get_impl(get, context);
            }
        };

        template<class T, class... Ids>
        struct statement_serializer<get_pointer_t<T, Ids...>, void> {
            using statement_type = get_pointer_t<T, Ids...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                return serialize_get_impl(statement, context);
            }
        };

        template<>
        struct statement_serializer<conflict_action, void> {
            using statement_type = conflict_action;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                switch(statement) {
                    case conflict_action::replace:
                        return "REPLACE";
                    case conflict_action::abort:
                        return "ABORT";
                    case conflict_action::fail:
                        return "FAIL";
                    case conflict_action::ignore:
                        return "IGNORE";
                    case conflict_action::rollback:
                        return "ROLLBACK";
                }
            }
        };

        template<>
        struct statement_serializer<insert_constraint, void> {
            using statement_type = insert_constraint;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                return "OR " + serialize(statement.action, context);
            }
        };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Ids>
        struct statement_serializer<get_optional_t<T, Ids...>, void> {
            using statement_type = get_optional_t<T, Ids...>;

            template<class Ctx>
            std::string operator()(const statement_type& get, const Ctx& context) const {
                return serialize_get_impl(get, context);
            }
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Args>
        struct statement_serializer<select_t<T, Args...>, void> {
            using statement_type = select_t<T, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& sel, const Ctx& context) const {
                std::stringstream ss;
                const auto isCompoundOperator = is_base_of_template<T, compound_operator>::value;
                if(!isCompoundOperator) {
                    if(!sel.highest_level && context.use_parentheses) {
                        ss << "(";
                    }
                    ss << "SELECT ";
                }
                if(get_distinct(sel.col)) {
                    ss << static_cast<std::string>(distinct(0)) << " ";
                }
                auto columnNames = get_column_names(sel.col, context);
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }
                }
                table_name_collector collector([&context](std::type_index ti) {
                    return context.impl.find_table_name(ti);
                });
                const auto explicitFromItemsCount = count_tuple<std::tuple<Args...>, is_from>::value;
                if(!explicitFromItemsCount) {
                    iterate_ast(sel.col, collector);
                    iterate_ast(sel.conditions, collector);
                    join_iterator<Args...>()([&collector, &context](const auto& c) {
                        using original_join_type = typename std::decay<decltype(c)>::type::join_type::type;
                        using cross_join_type = typename internal::mapped_type_proxy<original_join_type>::type;
                        auto crossJoinedTableName = context.impl.find_table_name(typeid(cross_join_type));
                        auto tableAliasString = alias_extractor<original_join_type>::get();
                        std::pair<std::string, std::string> tableNameWithAlias(std::move(crossJoinedTableName),
                                                                               std::move(tableAliasString));
                        collector.table_names.erase(tableNameWithAlias);
                    });
                    if(!collector.table_names.empty() && !isCompoundOperator) {
                        ss << " FROM ";
                        std::vector<std::pair<std::string, std::string>> tableNames(collector.table_names.begin(),
                                                                                    collector.table_names.end());
                        for(size_t i = 0; i < tableNames.size(); ++i) {
                            auto& tableNamePair = tableNames[i];
                            ss << "'" << tableNamePair.first << "'";
                            if(!tableNamePair.second.empty()) {
                                ss << " '" << tableNamePair.second << "'";
                            }
                            if(int(i) < int(tableNames.size()) - 1) {
                                ss << ", ";
                            }
                        }
                    }
                }
                iterate_tuple(sel.conditions, [&context, &ss](auto& v) {
                    ss << ' ' << serialize(v, context);
                });
                if(!is_base_of_template<T, compound_operator>::value) {
                    if(!sel.highest_level && context.use_parentheses) {
                        ss << ")";
                    }
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<indexed_column_t<T>, void> {
            using statement_type = indexed_column_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << serialize(statement.column_or_expression, context);
                if(!statement._collation_name.empty()) {
                    ss << " COLLATE " << statement._collation_name;
                }
                if(statement._order) {
                    switch(statement._order) {
                        case -1:
                            ss << " DESC";
                            break;
                        case 1:
                            ss << " ASC";
                            break;
                        default:
                            throw std::system_error{orm_error_code::incorrect_order};
                    }
                }
                return ss.str();
            }
        };

        template<class... Cols>
        struct statement_serializer<index_t<Cols...>, void> {
            using statement_type = index_t<Cols...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "CREATE ";
                if(statement.unique) {
                    ss << "UNIQUE ";
                }
                using elements_type = typename std::decay<decltype(statement)>::type::elements_type;
                using head_t = typename std::tuple_element<0, elements_type>::type::column_type;
                using indexed_type = typename table_type<head_t>::type;
                ss << "INDEX IF NOT EXISTS '" << statement.name << "' ON '"
                   << context.impl.find_table_name(typeid(indexed_type)) << "' (";
                std::vector<std::string> columnNames;
                std::string whereString;
                iterate_tuple(statement.elements, [&columnNames, &context, &whereString](auto& value) {
                    using value_type = typename std::decay<decltype(value)>::type;
                    if(!is_where<value_type>::value) {
                        auto newContext = context;
                        newContext.use_parentheses = false;
                        auto whereString = serialize(value, newContext);
                        columnNames.push_back(move(whereString));
                    } else {
                        auto columnName = serialize(value, context);
                        whereString = move(columnName);
                    }
                });
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")";
                if(!whereString.empty()) {
                    ss << ' ' << whereString;
                }
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<from_t<Args...>, void> {
            using statement_type = from_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using tuple = std::tuple<Args...>;

                std::stringstream ss;
                ss << "FROM ";
                size_t index = 0;
                iterate_tuple<tuple>([&context, &ss, &index](auto* itemPointer) {
                    using mapped_type = typename std::remove_pointer<decltype(itemPointer)>::type;

                    ss << "'" << context.impl.find_table_name(typeid(typename mapped_type_proxy<mapped_type>::type))
                       << "'";
                    auto aliasString = alias_extractor<mapped_type>::get();
                    if(!aliasString.empty()) {
                        ss << " '" << aliasString << "'";
                    }
                    if(index < std::tuple_size<tuple>::value - 1) {
                        ss << ", ";
                    }
                    ++index;
                });
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<old_t<T>, void> {
            using statement_type = old_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "OLD.";
                auto newContext = context;
                newContext.skip_table_name = true;
                ss << serialize(statement.expression, newContext);
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<new_t<T>, void> {
            using statement_type = new_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "NEW.";
                auto newContext = context;
                newContext.skip_table_name = true;
                ss << serialize(statement.expression, newContext);
                return ss.str();
            }
        };

        template<>
        struct statement_serializer<raise_t, void> {
            using statement_type = raise_t;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                switch(statement.type) {
                    case decltype(statement.type)::ignore:
                        return "RAISE(IGNORE)";

                    case decltype(statement.type)::rollback:
                        return "RAISE(ROLLBACK, " + serialize(statement.message, context) + ")";

                    case decltype(statement.type)::abort:
                        return "RAISE(ABORT, " + serialize(statement.message, context) + ")";

                    case decltype(statement.type)::fail:
                        return "RAISE(FAIL, " + serialize(statement.message, context) + ")";
                }
                return {};
            }
        };

        template<>
        struct statement_serializer<trigger_timing, void> {
            using statement_type = trigger_timing;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                switch(statement) {
                    case trigger_timing::trigger_before:
                        return "BEFORE";
                    case trigger_timing::trigger_after:
                        return "AFTER";
                    case trigger_timing::trigger_instead_of:
                        return "INSTEAD OF";
                    default:
                        return "";
                }
            }
        };

        template<>
        struct statement_serializer<trigger_type, void> {
            using statement_type = trigger_type;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                switch(statement) {
                    case trigger_type::trigger_delete:
                        return "DELETE";
                    case trigger_type::trigger_insert:
                        return "INSERT";
                    case trigger_type::trigger_update:
                        return "UPDATE";
                    default:
                        return "";
                }
            }
        };

        template<>
        struct statement_serializer<trigger_type_base_t, void> {
            using statement_type = trigger_type_base_t;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;

                ss << serialize(statement.timing, context) << " " << serialize(statement.type, context);
                return ss.str();
            }
        };

        template<class... Cs>
        struct statement_serializer<trigger_update_type_t<Cs...>, void> {
            using statement_type = trigger_update_type_t<Cs...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                std::string sep = "";

                ss << serialize(statement.timing, context) << " UPDATE OF ";
                iterate_tuple(statement.columns, [&ss, &sep, &context](auto& v) {
                    auto name = find_column_name(context.impl, v);

                    if(name == nullptr)
                        throw std::system_error{orm_error_code::column_not_found};
                    ss << sep << "'" << *name << "'";
                    sep = ", ";
                });
                return ss.str();
            }
        };

        template<class T, class W, class Trigger>
        struct statement_serializer<trigger_base_t<T, W, Trigger>, void> {
            using statement_type = trigger_base_t<T, W, Trigger>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;

                ss << serialize(statement.type_base, context);
                ss << " ON '" << context.impl.find_table_name(typeid(T)) << "'";
                if(statement.do_for_each_row) {
                    ss << " FOR EACH ROW";
                }
                statement.container_when.apply([&ss, &context](auto& value) {
                    ss << " WHEN " << serialize(value, context);
                });
                return ss.str();
            }
        };

        template<class... S>
        struct statement_serializer<trigger_t<S...>, void> {
            using statement_type = trigger_t<S...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "CREATE ";

                ss << "TRIGGER IF NOT EXISTS '" << statement.name << "' " << serialize(statement.base, context);
                ss << " BEGIN ";
                iterate_tuple(statement.elements, [&ss, &context](auto& element) {
                    using element_type = typename std::decay<decltype(element)>::type;
                    if(is_select<element_type>::value) {
                        auto newContext = context;
                        newContext.use_parentheses = false;
                        ss << serialize(element, newContext);
                    } else {
                        ss << serialize(element, context);
                    }
                    ss << ";";
                });
                ss << " END";

                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<where_t<T>, void> {
            using statement_type = where_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << statement.serialize() << " ";
                auto whereString = serialize(statement.expression, context);
                ss << '(' << whereString << ')';
                return ss.str();
            }
        };

        template<class O>
        struct statement_serializer<order_by_t<O>, void> {
            using statement_type = order_by_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& orderBy, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(orderBy) << " ";
                ss << serialize_order_by(orderBy, context);
                return ss.str();
            }
        };

        template<class C>
        struct statement_serializer<dynamic_order_by_t<C>, void> {
            using statement_type = dynamic_order_by_t<C>;

            template<class Ctx>
            std::string operator()(const statement_type& orderBy, const Ctx& context) const {
                return serialize_order_by(orderBy, context);
            }
        };

        template<class... Args>
        struct statement_serializer<multi_order_by_t<Args...>, void> {
            using statement_type = multi_order_by_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& orderBy, const Ctx& context) const {
                std::stringstream ss;
                std::vector<std::string> expressions;
                iterate_tuple(orderBy.args, [&expressions, &context](auto& v) {
                    auto expression = serialize_order_by(v, context);
                    expressions.push_back(move(expression));
                });
                ss << static_cast<std::string>(orderBy) << " ";
                for(size_t i = 0; i < expressions.size(); ++i) {
                    ss << expressions[i];
                    if(i < expressions.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << " ";
                return ss.str();
            }
        };

        template<class O>
        struct statement_serializer<cross_join_t<O>, void> {
            using statement_type = cross_join_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c);
                ss << " '" << context.impl.find_table_name(typeid(O)) << "'";
                return ss.str();
            }
        };

        template<class T, class O>
        struct statement_serializer<inner_join_t<T, O>, void> {
            using statement_type = inner_join_t<T, O>;

            template<class Ctx>
            std::string operator()(const statement_type& l, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(l);
                ss << " '" << context.impl.find_table_name(typeid(typename mapped_type_proxy<T>::type)) << "' ";
                auto aliasString = alias_extractor<T>::get();
                if(!aliasString.empty()) {
                    ss << "'" << aliasString << "' ";
                }
                ss << serialize(l.constraint, context);
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<on_t<T>, void> {
            using statement_type = on_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& t, const Ctx& context) const {
                std::stringstream ss;
                auto newContext = context;
                newContext.skip_table_name = false;
                ss << static_cast<std::string>(t) << " " << serialize(t.arg, newContext) << " ";
                return ss.str();
            }
        };

        template<class T, class O>
        struct statement_serializer<join_t<T, O>, void> {
            using statement_type = join_t<T, O>;

            template<class Ctx>
            std::string operator()(const statement_type& l, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(l);
                ss << " '" << context.impl.find_table_name(typeid(typename mapped_type_proxy<T>::type)) << "' ";
                auto aliasString = alias_extractor<T>::get();
                if(!aliasString.empty()) {
                    ss << "'" << aliasString << "' ";
                }
                ss << serialize(l.constraint, context);
                return ss.str();
            }
        };

        template<class T, class O>
        struct statement_serializer<left_join_t<T, O>, void> {
            using statement_type = left_join_t<T, O>;

            template<class Ctx>
            std::string operator()(const statement_type& l, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(l);
                ss << " '" << context.impl.find_table_name(typeid(typename mapped_type_proxy<T>::type)) << "' ";
                auto aliasString = alias_extractor<T>::get();
                if(!aliasString.empty()) {
                    ss << "'" << aliasString << "' ";
                }
                ss << serialize(l.constraint, context);
                return ss.str();
            }
        };

        template<class T, class O>
        struct statement_serializer<left_outer_join_t<T, O>, void> {
            using statement_type = left_outer_join_t<T, O>;

            template<class Ctx>
            std::string operator()(const statement_type& l, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(l);
                ss << " '" << context.impl.find_table_name(typeid(typename mapped_type_proxy<T>::type)) << "' ";
                auto aliasString = alias_extractor<T>::get();
                if(!aliasString.empty()) {
                    ss << "'" << aliasString << "' ";
                }
                ss << serialize(l.constraint, context);
                return ss.str();
            }
        };

        template<class O>
        struct statement_serializer<natural_join_t<O>, void> {
            using statement_type = natural_join_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c);
                ss << " '" << context.impl.find_table_name(typeid(O)) << "'";
                return ss.str();
            }
        };

        template<class T, class... Args>
        struct statement_serializer<group_by_with_having<T, Args...>, void> {
            using statement_type = group_by_with_having<T, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                std::vector<std::string> expressions;
                auto newContext = context;
                newContext.skip_table_name = false;
                iterate_tuple(statement.args, [&expressions, &newContext](auto& v) {
                    auto expression = serialize(v, newContext);
                    expressions.push_back(expression);
                });
                ss << "GROUP BY";
                for(size_t i = 0; i < expressions.size(); ++i) {
                    ss << ' ' << expressions[i];
                    if(i < expressions.size() - 1) {
                        ss << ",";
                    }
                }
                ss << " HAVING";
                ss << ' ' << serialize(statement.expression, context);
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<group_by_t<Args...>, void> {
            using statement_type = group_by_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                std::vector<std::string> expressions;
                auto newContext = context;
                newContext.skip_table_name = false;
                iterate_tuple(statement.args, [&expressions, &newContext](auto& v) {
                    auto expression = serialize(v, newContext);
                    expressions.push_back(expression);
                });
                ss << "GROUP BY";
                for(size_t i = 0; i < expressions.size(); ++i) {
                    ss << ' ' << expressions[i];
                    if(i < expressions.size() - 1) {
                        ss << ",";
                    }
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<having_t<T>, void> {
            using statement_type = having_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto newContext = context;
                newContext.skip_table_name = false;
                ss << "HAVING";
                ss << " " << serialize(statement.expression, newContext);
                return ss.str();
            }
        };

        /**
         *  HO - has offset
         *  OI - offset is implicit
         */
        template<class T, bool HO, bool OI, class O>
        struct statement_serializer<limit_t<T, HO, OI, O>, void> {
            using statement_type = limit_t<T, HO, OI, O>;

            template<class Ctx>
            std::string operator()(const statement_type& limt, const Ctx& context) const {
                auto newContext = context;
                newContext.skip_table_name = false;
                std::stringstream ss;
                ss << static_cast<std::string>(limt) << " ";
                if(HO) {
                    if(OI) {
                        limt.off.apply([&newContext, &ss](auto& value) {
                            ss << serialize(value, newContext);
                        });
                        ss << ", ";
                        ss << serialize(limt.lim, newContext);
                    } else {
                        ss << serialize(limt.lim, newContext) << " OFFSET ";
                        limt.off.apply([&newContext, &ss](auto& value) {
                            ss << serialize(value, newContext);
                        });
                    }
                } else {
                    ss << serialize(limt.lim, newContext);
                }
                return ss.str();
            }
        };

        template<>
        struct statement_serializer<default_values_t, void> {
            using statement_type = default_values_t;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                return "DEFAULT VALUES";
            }
        };

        template<class T, class M>
        struct statement_serializer<using_t<T, M>, void> {
            using statement_type = using_t<T, M>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                auto newContext = context;
                newContext.skip_table_name = true;
                return static_cast<std::string>(statement) + " (" + serialize(statement.column, newContext) + ")";
            }
        };

        template<class... Args>
        struct statement_serializer<std::tuple<Args...>, void> {
            using statement_type = std::tuple<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << '(';
                auto index = 0;
                using TupleSize = std::tuple_size<statement_type>;
                iterate_tuple(statement, [&context, &index, &ss](auto& value) {
                    ss << serialize(value, context);
                    if(index < TupleSize::value - 1) {
                        ss << ", ";
                    }
                    ++index;
                });
                ss << ')';
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<values_t<Args...>, void> {
            using statement_type = values_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                if(context.use_parentheses) {
                    ss << '(';
                }
                ss << "VALUES ";
                {
                    auto index = 0;
                    auto& tuple = statement.tuple;
                    using tuple_type = typename std::decay<decltype(tuple)>::type;
                    using TupleSize = std::tuple_size<tuple_type>;
                    iterate_tuple(tuple, [&context, &index, &ss](auto& value) {
                        ss << serialize(value, context);
                        if(index < TupleSize::value - 1) {
                            ss << ", ";
                        }
                        ++index;
                    });
                }
                if(context.use_parentheses) {
                    ss << ')';
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<dynamic_values_t<T>, void> {
            using statement_type = dynamic_values_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                if(context.use_parentheses) {
                    ss << '(';
                }
                ss << "VALUES ";
                {
                    auto vectorSize = statement.vector.size();
                    for(decltype(vectorSize) index = 0; index < vectorSize; ++index) {
                        auto& value = statement.vector[index];
                        ss << serialize(value, context);
                        if(index < vectorSize - 1) {
                            ss << ", ";
                        }
                    }
                }
                if(context.use_parentheses) {
                    ss << ')';
                }
                return ss.str();
            }
        };
    }
}
