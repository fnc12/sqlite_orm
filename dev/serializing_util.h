#pragma once

#include <type_traits>  //  std::index_sequence
#include <tuple>
#include <array>
#include <string>
#include <ostream>
#include <utility>  //  std::exchange, std::tuple_size

#include "functional/cxx_universal.h"  //  ::size_t
#include "functional/cxx_type_traits_polyfill.h"
#include "tuple_helper/tuple_iteration.h"
#include "error_code.h"
#include "serializer_context.h"
#include "serialize_result_type.h"
#include "util.h"

namespace sqlite_orm {
    namespace internal {
        template<class O>
        struct order_by_t;

        template<class T, class I>
        std::string serialize(const T& t, const serializer_context<I>& context);

        template<class T, class Ctx>
        std::string serialize_order_by(const T& t, const Ctx& context);

        inline void stream_sql_escaped(std::ostream& os, serialize_arg_type str, char char2Escape) {
            for(size_t offset = 0, next; true; offset = next + 1) {
                next = str.find(char2Escape, offset);

                if(next == str.npos) {
                    os.write(str.data() + offset, str.size() - offset);
                    break;
                }

                os.write(str.data() + offset, next - offset + 1);
                os.write(&char2Escape, 1);
            }
        }

        inline void stream_identifier(std::ostream& ss,
                                      serialize_arg_type qualifier,
                                      serialize_arg_type identifier,
                                      serialize_arg_type alias) {
            constexpr char quoteChar = '"';
            constexpr char qualified[] = {quoteChar, '.', '\0'};
            constexpr char aliased[] = {' ', quoteChar, '\0'};

            // note: In practice, escaping double quotes in identifiers is arguably overkill,
            // but since the SQLite grammar allows it, it's better to be safe than sorry.

            if(!qualifier.empty()) {
                ss << quoteChar;
                stream_sql_escaped(ss, qualifier, quoteChar);
                ss << qualified;
            }
            {
                ss << quoteChar;
                stream_sql_escaped(ss, identifier, quoteChar);
                ss << quoteChar;
            }
            if(!alias.empty()) {
                ss << aliased;
                stream_sql_escaped(ss, alias, quoteChar);
                ss << quoteChar;
            }
        }

        inline void stream_identifier(std::ostream& ss, const std::string& identifier, const std::string& alias) {
            return stream_identifier(ss, "", identifier, alias);
        }

        inline void stream_identifier(std::ostream& ss, const std::string& identifier) {
            return stream_identifier(ss, "", identifier, "");
        }

        template<typename Tpl, size_t... Is>
        void stream_identifier(std::ostream& ss, const Tpl& tpl, std::index_sequence<Is...>) {
            static_assert(sizeof...(Is) > 0 && sizeof...(Is) <= 3, "");
            return stream_identifier(ss, std::get<Is>(tpl)...);
        }

        template<typename Tpl, std::enable_if_t<polyfill::is_detected_v<type_t, std::tuple_size<Tpl>>, bool> = true>
        void stream_identifier(std::ostream& ss, const Tpl& tpl) {
            return stream_identifier(ss, tpl, std::make_index_sequence<std::tuple_size<Tpl>::value>{});
        }

        enum class stream_as {
            conditions_tuple,
            actions_tuple,
            expressions_tuple,
            dynamic_expressions,
            serialized,
            identifier,
            identifiers,
            values_placeholders,
            table_columns,
            non_generated_columns,
            field_values_excluding,
            mapped_columns_expressions,
            column_constraints,
        };

        template<stream_as mode>
        struct streaming {
            template<class... Ts>
            auto operator()(const Ts&... ts) const {
                return std::forward_as_tuple(*this, ts...);
            }

            template<size_t... Idx>
            constexpr std::index_sequence<1u + Idx...> offset_index(std::index_sequence<Idx...>) const {
                return {};
            }
        };
        constexpr streaming<stream_as::conditions_tuple> streaming_conditions_tuple{};
        constexpr streaming<stream_as::actions_tuple> streaming_actions_tuple{};
        constexpr streaming<stream_as::expressions_tuple> streaming_expressions_tuple{};
        constexpr streaming<stream_as::dynamic_expressions> streaming_dynamic_expressions{};
        constexpr streaming<stream_as::serialized> streaming_serialized{};
        constexpr streaming<stream_as::identifier> streaming_identifier{};
        constexpr streaming<stream_as::identifiers> streaming_identifiers{};
        constexpr streaming<stream_as::values_placeholders> streaming_values_placeholders{};
        constexpr streaming<stream_as::table_columns> streaming_table_column_names{};
        constexpr streaming<stream_as::non_generated_columns> streaming_non_generated_column_names{};
        constexpr streaming<stream_as::field_values_excluding> streaming_field_values_excluding{};
        constexpr streaming<stream_as::mapped_columns_expressions> streaming_mapped_columns_expressions{};
        constexpr streaming<stream_as::column_constraints> streaming_column_constraints{};

        // serialize and stream a tuple of condition expressions;
        // space + space-separated
        template<class T, class Ctx>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::conditions_tuple>&, T, Ctx> tpl) {
            const auto& conditions = get<1>(tpl);
            auto& context = get<2>(tpl);

            iterate_tuple(conditions, [&ss, &context](auto& c) {
                ss << " " << serialize(c, context);
            });
            return ss;
        }

        // serialize and stream a tuple of action expressions;
        // space-separated
        template<class T, class Ctx>
        std::ostream& operator<<(std::ostream& ss, std::tuple<const streaming<stream_as::actions_tuple>&, T, Ctx> tpl) {
            const auto& actions = get<1>(tpl);
            auto& context = get<2>(tpl);

            iterate_tuple(actions, [&ss, &context, first = true](auto& action) mutable {
                constexpr std::array<const char*, 2> sep = {" ", ""};
                ss << sep[std::exchange(first, false)] << serialize(action, context);
            });
            return ss;
        }

        // serialize and stream a tuple of expressions;
        // comma-separated
        template<class T, class Ctx>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::expressions_tuple>&, T, Ctx> tpl) {
            const auto& args = get<1>(tpl);
            auto& context = get<2>(tpl);

            iterate_tuple(args, [&ss, &context, first = true](auto& arg) mutable {
                constexpr std::array<const char*, 2> sep = {", ", ""};
                ss << sep[std::exchange(first, false)] << serialize(arg, context);
            });
            return ss;
        }

        // serialize and stream multi_order_by arguments;
        // comma-separated
        template<class... Os, class Ctx>
        std::ostream& operator<<(
            std::ostream& ss,
            std::tuple<const streaming<stream_as::expressions_tuple>&, const std::tuple<order_by_t<Os>...>&, Ctx> tpl) {
            const auto& args = get<1>(tpl);
            auto& context = get<2>(tpl);

            iterate_tuple(args, [&ss, &context, first = true](auto& arg) mutable {
                constexpr std::array<const char*, 2> sep = {", ", ""};
                ss << sep[std::exchange(first, false)] << serialize_order_by(arg, context);
            });
            return ss;
        }

        // serialize and stream a vector of expressions;
        // comma-separated
        template<class C, class Ctx>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::dynamic_expressions>&, C, Ctx> tpl) {
            const auto& args = get<1>(tpl);
            auto& context = get<2>(tpl);

            constexpr std::array<const char*, 2> sep = {", ", ""};
            for(size_t i = 0, first = true; i < args.size(); ++i) {
                ss << sep[std::exchange(first, false)] << serialize(args[i], context);
            }
            return ss;
        }

        // stream a vector of already serialized strings;
        // comma-separated
        template<class C>
        std::ostream& operator<<(std::ostream& ss, std::tuple<const streaming<stream_as::serialized>&, C> tpl) {
            const auto& strings = get<1>(tpl);

            constexpr std::array<const char*, 2> sep = {", ", ""};
            for(size_t i = 0, first = true; i < strings.size(); ++i) {
                ss << sep[std::exchange(first, false)] << strings[i];
            }
            return ss;
        }

        // stream an identifier described by a variadic string pack, which is one of:
        // 1. identifier
        // 2. identifier, alias
        // 3. qualifier, identifier, alias
        template<class... Strings>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::identifier>&, Strings...> tpl) {
            stream_identifier(ss, tpl, streaming_identifier.offset_index(std::index_sequence_for<Strings...>{}));
            return ss;
        }

        // stream a container of identifiers described by a string or a tuple, which is one of:
        // 1. identifier
        // 1. tuple(identifier)
        // 2. tuple(identifier, alias), pair(identifier, alias)
        // 3. tuple(qualifier, identifier, alias)
        //
        // comma-separated
        template<class C>
        std::ostream& operator<<(std::ostream& ss, std::tuple<const streaming<stream_as::identifiers>&, C> tpl) {
            const auto& identifiers = get<1>(tpl);

            constexpr std::array<const char*, 2> sep = {", ", ""};
            bool first = true;
            for(auto& identifier: identifiers) {
                ss << sep[std::exchange(first, false)];
                stream_identifier(ss, identifier);
            }
            return ss;
        }

        // stream placeholders as part of a values clause
        template<class... Ts>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::values_placeholders>&, Ts...> tpl) {
            const size_t& columnsCount = get<1>(tpl);
            const ptrdiff_t& valuesCount = get<2>(tpl);

            if(!valuesCount || !columnsCount) {
                return ss;
            }

            std::string result;
            result.reserve((1 + (columnsCount * 1) + (columnsCount * 2 - 2) + 1) * valuesCount + (valuesCount * 2 - 2));

            constexpr std::array<const char*, 2> sep = {", ", ""};
            for(ptrdiff_t i = 0, first = true; i < valuesCount; ++i) {
                result += sep[std::exchange(first, false)];
                result += "(";
                for(size_t i = 0, first = true; i < columnsCount; ++i) {
                    result += sep[std::exchange(first, false)];
                    result += "?";
                }
                result += ")";
            }
            ss << result;
            return ss;
        }

        // stream a table's column identifiers, possibly qualified;
        // comma-separated
        template<class Table>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::table_columns>&, Table, const bool&> tpl) {
            const auto& table = get<1>(tpl);
            const bool& qualified = get<2>(tpl);

            table.for_each_column([&ss, &tableName = qualified ? table.name : std::string{}, first = true](
                                      const column_identifier& column) mutable {
                constexpr std::array<const char*, 2> sep = {", ", ""};
                ss << sep[std::exchange(first, false)];
                stream_identifier(ss, tableName, column.name, std::string{});
            });
            return ss;
        }

        // stream a table's non-generated column identifiers, unqualified;
        // comma-separated
        template<class Table>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::non_generated_columns>&, Table> tpl) {
            const auto& table = get<1>(tpl);

            table.template for_each_column_excluding<is_generated_always>(
                [&ss, first = true](const column_identifier& column) mutable {
                    constexpr std::array<const char*, 2> sep = {", ", ""};
                    ss << sep[std::exchange(first, false)];
                    stream_identifier(ss, column.name);
                });
            return ss;
        }

        // stream a table's non-generated column identifiers, unqualified;
        // comma-separated
        template<class PredFnCls, class L, class Ctx, class Obj>
        std::ostream&
        operator<<(std::ostream& ss,
                   std::tuple<const streaming<stream_as::field_values_excluding>&, PredFnCls, L, Ctx, Obj> tpl) {
            using check_if_excluded = polyfill::remove_cvref_t<std::tuple_element_t<1, decltype(tpl)>>;
            auto& excluded = get<2>(tpl);
            auto& context = get<3>(tpl);
            auto& object = get<4>(tpl);
            using object_type = polyfill::remove_cvref_t<decltype(object)>;
            auto& table = pick_table<object_type>(context.db_objects);

            table.template for_each_column_excluding<check_if_excluded>(call_as_template_base<column_field>(
                [&ss, &excluded, &context, &object, first = true](auto& column) mutable {
                    if(excluded(column)) {
                        return;
                    }

                    constexpr std::array<const char*, 2> sep = {", ", ""};
                    ss << sep[std::exchange(first, false)]
                       << serialize(polyfill::invoke(column.member_pointer, object), context);
                }));
            return ss;
        }

        // stream a tuple of mapped columns (which are member pointers or column pointers);
        // comma-separated
        template<class T, class Ctx>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::mapped_columns_expressions>&, T, Ctx> tpl) {
            const auto& columns = get<1>(tpl);
            auto& context = get<2>(tpl);

            iterate_tuple(columns, [&ss, &context, first = true](auto& colRef) mutable {
                const std::string* columnName = find_column_name(context.db_objects, colRef);
                if(!columnName) {
                    throw std::system_error{orm_error_code::column_not_found};
                }

                constexpr std::array<const char*, 2> sep = {", ", ""};
                ss << sep[std::exchange(first, false)];
                stream_identifier(ss, *columnName);
            });
            return ss;
        }

        template<class... Op, class Ctx>
        std::ostream& operator<<(std::ostream& ss,
                                 std::tuple<const streaming<stream_as::column_constraints>&,
                                            const column_constraints<Op...>&,
                                            const bool&,
                                            Ctx> tpl) {
            const auto& column = get<1>(tpl);
            const bool& isNotNull = get<2>(tpl);
            auto& context = get<3>(tpl);

            using constraints_type = constraints_type_t<column_constraints<Op...>>;
            constexpr size_t constraintsCount = std::tuple_size<constraints_type>::value;
            if(constraintsCount) {
                std::vector<std::string> constraintsStrings;
                constraintsStrings.reserve(constraintsCount);
                int primaryKeyIndex = -1;
                int autoincrementIndex = -1;
                int tupleIndex = 0;
                iterate_tuple(column.constraints,
                              [&constraintsStrings, &primaryKeyIndex, &autoincrementIndex, &tupleIndex, &context](
                                  auto& constraint) {
                                  using constraint_type = std::decay_t<decltype(constraint)>;
                                  constraintsStrings.push_back(serialize(constraint, context));
                                  if(is_primary_key_v<constraint_type>) {
                                      primaryKeyIndex = tupleIndex;
                                  } else if(is_autoincrement_v<constraint_type>) {
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
            if(isNotNull) {
                ss << "NOT NULL ";
            }

            return ss;
        }
    }
}
