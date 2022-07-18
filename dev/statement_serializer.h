#pragma once

#include <sstream>  //  std::stringstream
#include <string>  //  std::string
#include <type_traits>  //  std::enable_if, std::remove_pointer
#include <vector>  //  std::vector
#include <algorithm>  //  std::iter_swap
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <codecvt>  //  std::codecvt_utf8_utf16
#endif  //  SQLITE_ORM_OMITS_CODECVT
#include <memory>
#include <array>
#include "functional/cxx_string_view.h"

#include "functional/cxx_universal.h"
#include "functional/cxx_functional_polyfill.h"
#include "functional/mpl.h"
#include "tuple_helper/tuple_filter.h"
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
#include "literal.h"
#include "table_name_collector.h"
#include "column_names_getter.h"
#include "order_by_serializer.h"
#include "serializing_util.h"
#include "statement_binder.h"
#include "values.h"
#include "triggers.h"
#include "table_type_of.h"
#include "index.h"
#include "util.h"

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
                return quote_string_literal(field_printer<std::string>{}(c));
            }

            std::string do_serialize(const char* c) const {
                return quote_string_literal(c);
            }
#ifndef SQLITE_ORM_OMITS_CODECVT
            std::string do_serialize(const std::wstring& c) const {
                // implementation detail: utilizing field_printer
                return quote_string_literal(field_printer<std::wstring>{}(c));
            }

            std::string do_serialize(const wchar_t* c) const {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return quote_string_literal(converter.to_bytes(c));
            }
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
            std::string do_serialize(const std::string_view& c) const {
                return quote_string_literal(std::string(c));
            }
#ifndef SQLITE_ORM_OMITS_CODECVT
            std::string do_serialize(const std::wstring_view& c) const {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return quote_string_literal(converter.to_bytes(c.data(), c.data() + c.size()));
            }
#endif
#endif
            /**
             *  Specialization for binary data (std::vector<char>).
             */
            std::string do_serialize(const std::vector<char>& t) const {
                return quote_blob_literal(field_printer<std::vector<char>>{}(t));
            }

            template<class P, class PT, class D>
            std::string do_serialize(const pointer_binding<P, PT, D>&) const {
                // always serialize null (security reasons)
                return field_printer<nullptr_t>{}(nullptr);
            }
        };

        /**
         *  Serializer for literal values.
         */
        template<class T>
        struct statement_serializer<T, internal::match_specialization_of<T, literal_holder>> {
            using statement_type = T;

            template<class Ctx>
            std::string operator()(const T& literal, const Ctx& context) const {
                static_assert(is_bindable_v<type_t<T>>, "A literal value must be also bindable");

                Ctx literalCtx = context;
                literalCtx.replace_bindable_with_question = false;
                statement_serializer<type_t<T>> serializer{};
                return serializer(literal.value, literalCtx);
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
                if(auto* columnName = find_column_name(context.db_objects, statement.expression)) {
                    ss << streaming_identifier(*columnName);
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
                std::stringstream ss;
                ss << streaming_identifier(T::get());
                return ss.str();
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
                    using value_type = std::decay_t<decltype(value)>;
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
                    auto updateContext = context;
                    updateContext.use_parentheses = false;
                    ss << " UPDATE " << streaming_actions_tuple(statement.actions, updateContext);
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
                ss << statement.serialize() << "(" << streaming_expressions_tuple(statement.args, context) << ")";
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
                std::stringstream ss;
                ss << F::name() << "(" << streaming_expressions_tuple(statement.args, context) << ")";
                return ss.str();
            }
        };

        template<class T, class E>
        struct statement_serializer<as_t<T, E>, void> {
            using statement_type = as_t<T, E>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << serialize(c.expression, context) + " AS " << streaming_identifier(alias_extractor<T>::get());
                return ss.str();
            }
        };

        template<class T, class P>
        struct statement_serializer<alias_column_t<T, P>, void> {
            using statement_type = alias_column_t<T, P>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                if(!context.skip_table_name) {
                    ss << streaming_identifier(alias_extractor<T>::get()) << ".";
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
                    ss << streaming_identifier(lookup_table_name<O>(context.db_objects)) << ".";
                }
                if(auto* columnName = find_column_name(context.db_objects, m)) {
                    ss << streaming_identifier(*columnName);
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
                    ss << streaming_identifier(lookup_table_name<O>(context.db_objects)) << ".";
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
                    ss << streaming_identifier(lookup_table_name<O>(context.db_objects)) << ".";
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
                    ss << streaming_identifier(lookup_table_name<O>(context.db_objects)) << ".";
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
                    ss << streaming_identifier(lookup_table_name<T>(context.db_objects)) << ".";
                }
                if(auto* columnName = find_column_name(context.db_objects, c)) {
                    ss << streaming_identifier(*columnName);
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
        struct statement_serializer<T, std::enable_if_t<is_base_of_template_v<T, compound_operator>>> {
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
        struct statement_serializer<T, std::enable_if_t<is_base_of_template_v<T, binary_condition>>> {
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
                constexpr bool isCompoundOperator = is_base_of_template_v<A, compound_operator>;
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
                ss << " (" << streaming_dynamic_expressions(statement.argument, context) << ")";
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
                using args_type = std::tuple<Args...>;
                constexpr bool theOnlySelect =
                    std::tuple_size<args_type>::value == 1 && is_select_v<std::tuple_element_t<0, args_type>>;
                if(!theOnlySelect) {
                    ss << "(";
                }
                ss << streaming_expressions_tuple(statement.argument, context);
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
            std::string operator()(const statement_type&, const Ctx&) const {
                return "AUTOINCREMENT";
            }
        };

        template<>
        struct statement_serializer<conflict_clause_t, void> {
            using statement_type = conflict_clause_t;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                switch(statement) {
                    case conflict_clause_t::rollback:
                        return "ROLLBACK";
                    case conflict_clause_t::abort:
                        return "ABORT";
                    case conflict_clause_t::fail:
                        return "FAIL";
                    case conflict_clause_t::ignore:
                        return "IGNORE";
                    case conflict_clause_t::replace:
                        return "REPLACE";
                }
                return {};
            }
        };

        template<class T>
        struct statement_serializer<primary_key_with_autoincrement<T>, void> {
            using statement_type = primary_key_with_autoincrement<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                return serialize(statement.primary_key, context) + " AUTOINCREMENT";
            }
        };

        template<class... Cs>
        struct statement_serializer<primary_key_t<Cs...>, void> {
            using statement_type = primary_key_t<Cs...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                ss << "PRIMARY KEY";
                switch(statement.options.asc_option) {
                    case statement_type::order_by::ascending:
                        ss << " ASC";
                        break;
                    case statement_type::order_by::descending:
                        ss << " DESC";
                        break;
                    default:
                        break;
                }
                if(statement.options.conflict_clause_is_on) {
                    ss << " ON CONFLICT " << serialize(statement.options.conflict_clause, context);
                }
                using columns_tuple = typename statement_type::columns_tuple;
                const size_t columnsCount = std::tuple_size<columns_tuple>::value;
                if(columnsCount) {
                    ss << "(" << streaming_mapped_columns_expressions(statement.columns, context) << ")";
                }
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<unique_t<Args...>, void> {
            using statement_type = unique_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c);
                using columns_tuple = typename statement_type::columns_tuple;
                const size_t columnsCount = std::tuple_size<columns_tuple>::value;
                if(columnsCount) {
                    ss << "(" << streaming_mapped_columns_expressions(c.columns, context) << ")";
                }
                return ss.str();
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
                ss << "FOREIGN KEY(" << streaming_mapped_columns_expressions(fk.columns, context) << ") REFERENCES ";
                {
                    using references_type_t = typename std::decay_t<decltype(fk)>::references_type;
                    using first_reference_t = std::tuple_element_t<0, references_type_t>;
                    using first_reference_mapped_type = table_type_of_t<first_reference_t>;
                    auto refTableName = lookup_table_name<first_reference_mapped_type>(context.db_objects);
                    ss << streaming_identifier(refTableName);
                }
                ss << "(" << streaming_mapped_columns_expressions(fk.references, context) << ")";
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
                std::stringstream ss;
                ss << "CHECK (" << serialize(statement.expression, context) << ")";
                return ss.str();
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
                    case basic_generated_always::storage_type::not_specified:
                        //..
                        break;
                    case basic_generated_always::storage_type::virtual_:
                        ss << " VIRTUAL";
                        break;
                    case basic_generated_always::storage_type::stored:
                        ss << " STORED";
                        break;
                }
                return ss.str();
            }
        };
#endif
        template<class G, class S, class... Op>
        struct statement_serializer<column_t<G, S, Op...>, void> {
            using statement_type = column_t<G, S, Op...>;

            template<class Ctx>
            std::string operator()(const statement_type& column, const Ctx& context) const {
                using column_type = statement_type;

                std::stringstream ss;
                ss << streaming_identifier(column.name) << " " << type_printer<field_type_t<column_type>>().print()
                   << " "
                   << streaming_column_constraints(
                          call_as_template_base<column_constraints>(polyfill::identity{})(column),
                          column.is_not_null(),
                          context);
                return ss.str();
            }
        };

        template<class T, class... Args>
        struct statement_serializer<remove_all_t<T, Args...>, void> {
            using statement_type = remove_all_t<T, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& rem, const Ctx& context) const {
                auto& table = pick_table<T>(context.db_objects);

                std::stringstream ss;
                ss << "DELETE FROM " << streaming_identifier(table.name)
                   << streaming_conditions_tuple(rem.conditions, context);
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<replace_t<T>, void> {
            using statement_type = replace_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using expression_type = std::decay_t<decltype(statement)>;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& table = pick_table<object_type>(context.db_objects);
                std::stringstream ss;
                ss << "REPLACE INTO " << streaming_identifier(table.name) << " ("
                   << streaming_non_generated_column_names(table) << ")"
                   << " VALUES ("
                   << streaming_field_values_excluding(check_if<is_generated_always>{},
                                                       empty_callable<std::false_type>(),  //  don't exclude
                                                       context,
                                                       get_ref(statement.object))
                   << ")";
                return ss.str();
            }
        };

        template<class T, class... Cols>
        struct statement_serializer<insert_explicit<T, Cols...>, void> {
            using statement_type = insert_explicit<T, Cols...>;

            template<class Ctx>
            std::string operator()(const statement_type& ins, const Ctx& context) const {
                constexpr size_t colsCount = std::tuple_size<std::tuple<Cols...>>::value;
                static_assert(colsCount > 0, "Use insert or replace with 1 argument instead");
                using expression_type = std::decay_t<decltype(ins)>;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& table = pick_table<object_type>(context.db_objects);
                std::stringstream ss;
                ss << "INSERT INTO " << streaming_identifier(table.name) << " ";
                ss << "(" << streaming_mapped_columns_expressions(ins.columns.columns, context) << ") "
                   << "VALUES (";
                iterate_tuple(ins.columns.columns,
                              [&ss, &context, &object = get_ref(ins.obj), first = true](auto& memberPointer) mutable {
                                  using member_pointer_type = std::decay_t<decltype(memberPointer)>;
                                  static_assert(!is_setter_v<member_pointer_type>,
                                                "Unable to use setter within insert explicit");

                                  constexpr std::array<const char*, 2> sep = {", ", ""};
                                  ss << sep[std::exchange(first, false)]
                                     << serialize(polyfill::invoke(memberPointer, object), context);
                              });
                ss << ")";
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<update_t<T>, void> {
            using statement_type = update_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using expression_type = std::decay_t<decltype(statement)>;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& table = pick_table<object_type>(context.db_objects);

                std::stringstream ss;
                ss << "UPDATE " << streaming_identifier(table.name) << " SET ";
                table.template for_each_column_excluding<mpl::disjunction_fn<is_primary_key, is_generated_always>>(
                    [&table, &ss, &context, &object = get_ref(statement.object), first = true](auto& column) mutable {
                        if(table.exists_in_composite_primary_key(column)) {
                            return;
                        }

                        constexpr std::array<const char*, 2> sep = {", ", ""};
                        ss << sep[std::exchange(first, false)] << streaming_identifier(column.name) << " = "
                           << serialize(polyfill::invoke(column.member_pointer, object), context);
                    });
                ss << " WHERE ";
                table.for_each_column(
                    [&table, &context, &ss, &object = get_ref(statement.object), first = true](auto& column) mutable {
                        if(!column.template is<is_primary_key>() && !table.exists_in_composite_primary_key(column)) {
                            return;
                        }

                        constexpr std::array<const char*, 2> sep = {" AND ", ""};
                        ss << sep[std::exchange(first, false)] << streaming_identifier(column.name) << " = "
                           << serialize(polyfill::invoke(column.member_pointer, object), context);
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
                ss << "SET ";
                auto leftContext = context;
                leftContext.skip_table_name = true;
                iterate_tuple(statement.assigns, [&ss, &context, &leftContext, first = true](auto& value) mutable {
                    constexpr std::array<const char*, 2> sep = {", ", ""};
                    ss << sep[std::exchange(first, false)] << serialize(value.lhs, leftContext) << ' '
                       << value.serialize() << ' ' << serialize(value.rhs, context);
                });
                return ss.str();
            }
        };

        template<class... Args, class... Wargs>
        struct statement_serializer<update_all_t<set_t<Args...>, Wargs...>, void> {
            using statement_type = update_all_t<set_t<Args...>, Wargs...>;

            template<class Ctx>
            std::string operator()(const statement_type& upd, const Ctx& context) const {
                table_name_collector collector([&context](const std::type_index& ti) {
                    return find_table_name(context.db_objects, ti);
                });
                iterate_ast(upd.set.assigns, collector);

                if(collector.table_names.empty()) {
                    throw std::system_error{orm_error_code::no_tables_specified};
                }

                std::stringstream ss;
                ss << "UPDATE " << streaming_identifier(collector.table_names.begin()->first) << " SET ";
                {
                    std::vector<std::string> setPairs;
                    setPairs.reserve(std::tuple_size<typename set_t<Args...>::assigns_type>::value);
                    auto leftContext = context;
                    leftContext.skip_table_name = true;
                    iterate_tuple(upd.set.assigns, [&context, &leftContext, &setPairs](auto& asgn) {
                        std::stringstream sss;
                        sss << serialize(asgn.lhs, leftContext);
                        sss << ' ' << asgn.serialize() << ' ';
                        sss << serialize(asgn.rhs, context);
                        setPairs.push_back(sss.str());
                    });
                    ss << streaming_serialized(setPairs) << streaming_conditions_tuple(upd.conditions, context);
                    return ss.str();
                }
            }
        };

        template<class T>
        struct statement_serializer<insert_t<T>, void> {
            using statement_type = insert_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using object_type = typename expression_object_type<statement_type>::type;
                auto& table = pick_table<object_type>(context.db_objects);
                using is_without_rowid = typename std::decay_t<decltype(table)>::is_without_rowid;

                std::vector<std::reference_wrapper<const std::string>> columnNames;
                table.template for_each_column_excluding<
                    mpl::conjunction<mpl::not_<mpl::always<is_without_rowid>>,
                                     mpl::disjunction_fn<is_primary_key, is_generated_always>>>(
                    [&table, &columnNames](auto& column) {
                        if(table.exists_in_composite_primary_key(column)) {
                            return;
                        }

                        columnNames.push_back(cref(column.name));
                    });
                const size_t columnNamesCount = columnNames.size();

                std::stringstream ss;
                ss << "INSERT INTO " << streaming_identifier(table.name) << " ";
                if(columnNamesCount) {
                    ss << "(" << streaming_identifiers(columnNames) << ")";
                } else {
                    ss << "DEFAULT";
                }
                ss << " VALUES";
                if(columnNamesCount) {
                    ss << " ("
                       << streaming_field_values_excluding(
                              mpl::conjunction<mpl::not_<mpl::always<is_without_rowid>>,
                                               mpl::disjunction_fn<is_primary_key, is_generated_always>>{},
                              [&table](auto& column) {
                                  return table.exists_in_composite_primary_key(column);
                              },
                              context,
                              get_ref(statement.object))
                       << ")";
                }

                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<into_t<T>, void> {
            using statement_type = into_t<T>;

            template<class Ctx>
            std::string operator()(const statement_type&, const Ctx& context) const {
                auto& table = pick_table<T>(context.db_objects);

                std::stringstream ss;
                ss << "INTO " << streaming_identifier(table.name);
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<columns_t<Args...>, void> {
            using statement_type = columns_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                if(context.use_parentheses) {
                    ss << '(';
                }
                ss << streaming_expressions_tuple(statement.columns, context);
                if(context.use_parentheses) {
                    ss << ')';
                }
                return ss.str();
            }
        };

        template<class T>
        struct statement_serializer<T, std::enable_if_t<polyfill::disjunction_v<is_insert_raw<T>, is_replace_raw<T>>>> {
            using statement_type = T;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                if(is_insert_raw_v<T>) {
                    ss << "INSERT";
                } else {
                    ss << "REPLACE";
                }
                iterate_tuple(statement.args, [&context, &ss](auto& value) {
                    using value_type = std::decay_t<decltype(value)>;
                    ss << ' ';
                    if(is_columns_v<value_type>) {
                        auto newContext = context;
                        newContext.skip_table_name = true;
                        newContext.use_parentheses = true;
                        ss << serialize(value, newContext);
                    } else if(is_values_v<value_type> || is_select_v<value_type>) {
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
                auto& table = pick_table<T>(context.db_objects);
                std::stringstream ss;
                ss << "DELETE FROM " << streaming_identifier(table.name) << " "
                   << "WHERE ";
                std::vector<std::string> idsStrings;
                idsStrings.reserve(std::tuple_size<typename statement_type::ids_type>::value);
                iterate_tuple(statement.ids, [&idsStrings, &context](auto& idValue) {
                    idsStrings.push_back(serialize(idValue, context));
                });
                table.for_each_primary_key_column([&table, &ss, &idsStrings, index = 0](auto& memberPointer) mutable {
                    auto* columnName = table.find_column_name(memberPointer);
                    if(!columnName) {
                        throw std::system_error{orm_error_code::column_not_found};
                    }

                    constexpr std::array<const char*, 2> sep = {" AND ", ""};
                    ss << sep[index == 0] << streaming_identifier(*columnName) << " = " << idsStrings[index];
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
                using expression_type = std::decay_t<decltype(rep)>;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& table = pick_table<object_type>(context.db_objects);

                std::stringstream ss;
                ss << "REPLACE INTO " << streaming_identifier(table.name) << " ("
                   << streaming_non_generated_column_names(table) << ")";
                const auto valuesCount = std::distance(rep.range.first, rep.range.second);
                const auto columnsCount = table.non_generated_columns_count();
                ss << " VALUES " << streaming_values_placeholders(columnsCount, valuesCount);
                return ss.str();
            }
        };

        template<class It, class L, class O>
        struct statement_serializer<insert_range_t<It, L, O>, void> {
            using statement_type = insert_range_t<It, L, O>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                using object_type = typename expression_object_type<statement_type>::type;
                auto& table = pick_table<object_type>(context.db_objects);
                using is_without_rowid = typename std::decay_t<decltype(table)>::is_without_rowid;

                std::vector<std::reference_wrapper<const std::string>> columnNames;
                table.template for_each_column_excluding<
                    mpl::conjunction<mpl::not_<mpl::always<is_without_rowid>>,
                                     mpl::disjunction_fn<is_primary_key, is_generated_always>>>(
                    [&table, &columnNames](auto& column) {
                        if(table.exists_in_composite_primary_key(column)) {
                            return;
                        }

                        columnNames.push_back(cref(column.name));
                    });
                const size_t valuesCount = std::distance(statement.range.first, statement.range.second);
                const size_t columnNamesCount = columnNames.size();

                std::stringstream ss;
                ss << "INSERT INTO " << streaming_identifier(table.name) << " ";
                if(columnNamesCount) {
                    ss << "(" << streaming_identifiers(columnNames) << ")";
                } else {
                    ss << "DEFAULT";
                }
                ss << " VALUES ";
                if(columnNamesCount) {
                    ss << streaming_values_placeholders(columnNamesCount, valuesCount);
                } else if(valuesCount != 1) {
                    throw std::system_error{orm_error_code::cannot_use_default_value};
                }
                return ss.str();
            }
        };

        template<class T, class Ctx>
        std::string serialize_get_all_impl(const T& get, const Ctx& context) {
            using primary_type = type_t<T>;

            table_name_collector collector;
            collector.table_names.emplace(lookup_table_name<primary_type>(context.db_objects), "");
            // note: not collecting table names from get.conditions;

            auto& table = pick_table<primary_type>(context.db_objects);
            std::stringstream ss;
            ss << "SELECT " << streaming_table_column_names(table, true);
            if(!collector.table_names.empty()) {
                ss << " FROM " << streaming_identifiers(collector.table_names);
            }
            ss << streaming_conditions_tuple(get.conditions, context);
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
            using primary_type = type_t<T>;
            auto& table = pick_table<primary_type>(context.db_objects);
            std::stringstream ss;
            ss << "SELECT " << streaming_table_column_names(table, false) << " FROM "
               << streaming_identifier(table.name) << " WHERE ";

            auto primaryKeyColumnNames = table.primary_key_column_names();
            if(primaryKeyColumnNames.empty()) {
                throw std::system_error{orm_error_code::table_has_no_primary_key_column};
            }

            for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                if(i > 0) {
                    ss << " AND ";
                }
                ss << streaming_identifier(primaryKeyColumnNames[i]) << " = ?";
            }
            return ss.str();
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
            std::string operator()(const statement_type& statement, const Ctx&) const {
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
                return {};
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
                constexpr bool isCompoundOperator = is_base_of_template_v<T, compound_operator>;
                if(!isCompoundOperator) {
                    if(!sel.highest_level && context.use_parentheses) {
                        ss << "(";
                    }
                    ss << "SELECT ";
                }
                if(get_distinct(sel.col)) {
                    ss << static_cast<std::string>(distinct(0)) << " ";
                }
                ss << streaming_serialized(get_column_names(sel.col, context));
                table_name_collector collector([&context](const std::type_index& ti) {
                    return find_table_name(context.db_objects, ti);
                });
                constexpr bool explicitFromItemsCount = count_tuple<std::tuple<Args...>, is_from>::value;
                if(!explicitFromItemsCount) {
                    iterate_ast(sel.col, collector);
                    iterate_ast(sel.conditions, collector);
                    join_iterator<Args...>()([&collector, &context](const auto& c) {
                        using original_join_type = typename std::decay_t<decltype(c)>::join_type::type;
                        using cross_join_type = mapped_type_proxy_t<original_join_type>;
                        auto crossJoinedTableName = lookup_table_name<cross_join_type>(context.db_objects);
                        auto tableAliasString = alias_extractor<original_join_type>::get();
                        std::pair<std::string, std::string> tableNameWithAlias{std::move(crossJoinedTableName),
                                                                               std::move(tableAliasString)};
                        collector.table_names.erase(tableNameWithAlias);
                    });
                    if(!collector.table_names.empty() && !isCompoundOperator) {
                        ss << " FROM " << streaming_identifiers(collector.table_names);
                    }
                }
                ss << streaming_conditions_tuple(sel.conditions, context);
                if(!is_base_of_template_v<T, compound_operator>) {
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
                using elements_type = typename std::decay_t<decltype(statement)>::elements_type;
                using head_t = typename std::tuple_element_t<0, elements_type>::column_type;
                using indexed_type = table_type_of_t<head_t>;
                ss << "INDEX IF NOT EXISTS " << streaming_identifier(statement.name) << " ON "
                   << streaming_identifier(lookup_table_name<indexed_type>(context.db_objects));
                std::vector<std::string> columnNames;
                std::string whereString;
                iterate_tuple(statement.elements, [&columnNames, &context, &whereString](auto& value) {
                    using value_type = std::decay_t<decltype(value)>;
                    if(!is_where_v<value_type>) {
                        auto newContext = context;
                        newContext.use_parentheses = false;
                        auto whereString = serialize(value, newContext);
                        columnNames.push_back(move(whereString));
                    } else {
                        auto columnName = serialize(value, context);
                        whereString = move(columnName);
                    }
                });
                ss << " (" << streaming_serialized(columnNames) << ")";
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
            std::string operator()(const statement_type&, const Ctx& context) const {
                using tuple = std::tuple<Args...>;

                std::stringstream ss;
                ss << "FROM ";
                iterate_tuple<tuple>([&context, &ss, first = true](auto* item) mutable {
                    using from_type = std::remove_pointer_t<decltype(item)>;

                    constexpr std::array<const char*, 2> sep = {", ", ""};
                    ss << sep[std::exchange(first, false)]
                       << streaming_identifier(lookup_table_name<mapped_type_proxy_t<from_type>>(context.db_objects),
                                               alias_extractor<from_type>::get());
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
                    case raise_t::type_t::ignore:
                        return "RAISE(IGNORE)";

                    case raise_t::type_t::rollback:
                        return "RAISE(ROLLBACK, " + serialize(statement.message, context) + ")";

                    case raise_t::type_t::abort:
                        return "RAISE(ABORT, " + serialize(statement.message, context) + ")";

                    case raise_t::type_t::fail:
                        return "RAISE(FAIL, " + serialize(statement.message, context) + ")";
                }
                return {};
            }
        };

        template<>
        struct statement_serializer<trigger_timing, void> {
            using statement_type = trigger_timing;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx&) const {
                switch(statement) {
                    case trigger_timing::trigger_before:
                        return "BEFORE";
                    case trigger_timing::trigger_after:
                        return "AFTER";
                    case trigger_timing::trigger_instead_of:
                        return "INSTEAD OF";
                }
                return {};
            }
        };

        template<>
        struct statement_serializer<trigger_type, void> {
            using statement_type = trigger_type;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx&) const {
                switch(statement) {
                    case trigger_type::trigger_delete:
                        return "DELETE";
                    case trigger_type::trigger_insert:
                        return "INSERT";
                    case trigger_type::trigger_update:
                        return "UPDATE";
                }
                return {};
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

                ss << serialize(statement.timing, context) << " UPDATE OF "
                   << streaming_mapped_columns_expressions(statement.columns, context);
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
                ss << " ON " << streaming_identifier(lookup_table_name<T>(context.db_objects));
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

                ss << "TRIGGER IF NOT EXISTS " << streaming_identifier(statement.name) << " "
                   << serialize(statement.base, context);
                ss << " BEGIN ";
                iterate_tuple(statement.elements, [&ss, &context](auto& element) {
                    using element_type = std::decay_t<decltype(element)>;
                    if(is_select_v<element_type>) {
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
                ss << static_cast<std::string>(orderBy) << " " << streaming_expressions_tuple(orderBy.args, context);
                return ss.str();
            }
        };

        template<class O>
        struct statement_serializer<cross_join_t<O>, void> {
            using statement_type = cross_join_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << " "
                   << streaming_identifier(lookup_table_name<O>(context.db_objects));
                return ss.str();
            }
        };

        template<class T, class O>
        struct statement_serializer<inner_join_t<T, O>, void> {
            using statement_type = inner_join_t<T, O>;

            template<class Ctx>
            std::string operator()(const statement_type& l, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(l) << " "
                   << streaming_identifier(lookup_table_name<mapped_type_proxy_t<T>>(context.db_objects),
                                           alias_extractor<T>::get())
                   << serialize(l.constraint, context);
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
                ss << static_cast<std::string>(l) << " "
                   << streaming_identifier(lookup_table_name<mapped_type_proxy_t<T>>(context.db_objects),
                                           alias_extractor<T>::get())
                   << " " << serialize(l.constraint, context);
                return ss.str();
            }
        };

        template<class T, class O>
        struct statement_serializer<left_join_t<T, O>, void> {
            using statement_type = left_join_t<T, O>;

            template<class Ctx>
            std::string operator()(const statement_type& l, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(l) << " "
                   << streaming_identifier(lookup_table_name<mapped_type_proxy_t<T>>(context.db_objects),
                                           alias_extractor<T>::get())
                   << " " << serialize(l.constraint, context);
                return ss.str();
            }
        };

        template<class T, class O>
        struct statement_serializer<left_outer_join_t<T, O>, void> {
            using statement_type = left_outer_join_t<T, O>;

            template<class Ctx>
            std::string operator()(const statement_type& l, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(l) << " "
                   << streaming_identifier(lookup_table_name<mapped_type_proxy_t<T>>(context.db_objects),
                                           alias_extractor<T>::get())
                   << " " << serialize(l.constraint, context);
                return ss.str();
            }
        };

        template<class O>
        struct statement_serializer<natural_join_t<O>, void> {
            using statement_type = natural_join_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& c, const Ctx& context) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << " "
                   << streaming_identifier(lookup_table_name<O>(context.db_objects));
                return ss.str();
            }
        };

        template<class T, class... Args>
        struct statement_serializer<group_by_with_having<T, Args...>, void> {
            using statement_type = group_by_with_having<T, Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto newContext = context;
                newContext.skip_table_name = false;
                ss << "GROUP BY " << streaming_expressions_tuple(statement.args, newContext) << " HAVING "
                   << serialize(statement.expression, context);
                return ss.str();
            }
        };

        template<class... Args>
        struct statement_serializer<group_by_t<Args...>, void> {
            using statement_type = group_by_t<Args...>;

            template<class Ctx>
            std::string operator()(const statement_type& statement, const Ctx& context) const {
                std::stringstream ss;
                auto newContext = context;
                newContext.skip_table_name = false;
                ss << "GROUP BY " << streaming_expressions_tuple(statement.args, newContext);
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
                ss << "HAVING " << serialize(statement.expression, newContext);
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
            std::string operator()(const statement_type&, const Ctx&) const {
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
                ss << '(' << streaming_expressions_tuple(statement, context) << ')';
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
                ss << "VALUES " << streaming_expressions_tuple(statement.tuple, context);
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
                ss << "VALUES " << streaming_dynamic_expressions(statement.vector, context);
                if(context.use_parentheses) {
                    ss << ')';
                }
                return ss.str();
            }
        };
    }
}
