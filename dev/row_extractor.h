#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::enable_if_t, std::is_arithmetic, std::is_same, std::enable_if
#include <cstdlib>  //  atof, atoi, atoll
#include <cstring>  //  strlen
#include <system_error>  //  std::system_error
#include <string>  //  std::string, std::wstring
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <locale>  // std::wstring_convert
#include <codecvt>  //  std::codecvt_utf8_utf16
#endif
#include <vector>  //  std::vector
#include <algorithm>  //  std::copy
#include <iterator>  //  std::back_inserter
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>
#endif

#include "functional/cxx_functional_polyfill.h"
#include "functional/static_magic.h"
#include "tuple_helper/tuple_transformer.h"
#include "column_result_proxy.h"
#include "arithmetic_tag.h"
#include "pointer_value.h"
#include "journal_mode.h"
#include "locking_mode.h"
#include "error_code.h"
#include "is_std_ptr.h"
#include "type_traits.h"

namespace sqlite_orm {

    /**
     *  Helper for casting values originating from SQL to C++ typed values, usually from rows of a result set.
     *  
     *  sqlite_orm provides specializations for known C++ types, users may define their custom specialization
     *  of this helper.
     *  
     *  @note (internal): Since row extractors are used in certain contexts with only one purpose at a time
     *                    (e.g., converting a row result set but not function values or column text),
     *                    there are factory functions that perform conceptual checking that should be used
     *                    instead of directly creating row extractors.
     *  
     *  
     */
    template<class V, typename Enable = void>
    struct row_extractor {
        /*
         *  Called during one-step query execution (one result row) for each column of a result row.
         */
        V extract(const char* columnText) const = delete;

        /*
         *  Called during multi-step query execution (result set) for each column of a result row.
         */
        V extract(sqlite3_stmt* stmt, int columnIndex) const = delete;

        /*
         *  Called before invocation of user-defined scalar or aggregate functions,
         *  in order to unbox dynamically typed SQL function values into a tuple of C++ function arguments.
         */
        V extract(sqlite3_value* value) const = delete;
    };

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<typename T>
    concept orm_column_text_extractable = requires(const row_extractor<T>& extractor, const char* columnText) {
        { extractor.extract(columnText) } -> std::same_as<T>;
    };

    template<typename T>
    concept orm_row_value_extractable =
        requires(const row_extractor<T>& extractor, sqlite3_stmt* stmt, int columnIndex) {
            { extractor.extract(stmt, columnIndex) } -> std::same_as<T>;
        };

    template<typename T>
    concept orm_boxed_value_extractable = requires(const row_extractor<T>& extractor, sqlite3_value* value) {
        { extractor.extract(value) } -> std::same_as<T>;
    };
#endif

    namespace internal {
        /*  
         *  Make a row extractor to be used for casting SQL column text to a C++ typed value.
         */
        template<class R>
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_column_text_extractable<R>)
#endif
        row_extractor<R> column_text_extractor() {
            return {};
        }

        /*  
         *  Make a row extractor to be used for converting a value from a SQL result row set to a C++ typed value.
         */
        template<class R>
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_row_value_extractable<R>)
#endif
        row_extractor<R> row_value_extractor() {
            return {};
        }

        /*  
         *  Make a row extractor to be used for unboxing a dynamically typed SQL value to a C++ typed value.
         */
        template<class R>
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_boxed_value_extractable<R>)
#endif
        row_extractor<R> boxed_value_extractor() {
            return {};
        }
    }

    template<class R>
    int extract_single_value(void* data, int argc, char** argv, char**) {
        auto& res = *(R*)data;
        if(argc) {
            const auto rowExtractor = internal::column_text_extractor<R>();
            res = rowExtractor.extract(argv[0]);
        }
        return 0;
    }

#if SQLITE_VERSION_NUMBER >= 3020000
    /**
     *  Specialization for the 'pointer-passing interface'.
     * 
     *  @note The 'pointer-passing' interface doesn't support (and in fact prohibits)
     *  extracting pointers from columns.
     */
    template<class P, class T>
    struct row_extractor<pointer_arg<P, T>, void> {
        using V = pointer_arg<P, T>;

        V extract(const char* columnText) const = delete;

        V extract(sqlite3_stmt* stmt, int columnIndex) const = delete;

        V extract(sqlite3_value* value) const {
            return {(P*)sqlite3_value_pointer(value, T::value)};
        }
    };

    /**
     * Undefine using pointer_binding<> for querying values
     */
    template<class P, class T, class D>
    struct row_extractor<pointer_binding<P, T, D>, void>;
#endif

    /**
     *  Specialization for arithmetic types.
     */
    template<class V>
    struct row_extractor<V, std::enable_if_t<std::is_arithmetic<V>::value>> {
        V extract(const char* columnText) const {
            return this->extract(columnText, tag());
        }

        V extract(sqlite3_stmt* stmt, int columnIndex) const {
            return this->extract(stmt, columnIndex, tag());
        }

        V extract(sqlite3_value* value) const {
            return this->extract(value, tag());
        }

      private:
        using tag = arithmetic_tag_t<V>;

        V extract(const char* columnText, const int_or_smaller_tag&) const {
            return static_cast<V>(atoi(columnText));
        }

        V extract(sqlite3_stmt* stmt, int columnIndex, const int_or_smaller_tag&) const {
            return static_cast<V>(sqlite3_column_int(stmt, columnIndex));
        }

        V extract(sqlite3_value* value, const int_or_smaller_tag&) const {
            return static_cast<V>(sqlite3_value_int(value));
        }

        V extract(const char* columnText, const bigint_tag&) const {
            return static_cast<V>(atoll(columnText));
        }

        V extract(sqlite3_stmt* stmt, int columnIndex, const bigint_tag&) const {
            return static_cast<V>(sqlite3_column_int64(stmt, columnIndex));
        }

        V extract(sqlite3_value* value, const bigint_tag&) const {
            return static_cast<V>(sqlite3_value_int64(value));
        }

        V extract(const char* columnText, const real_tag&) const {
            return static_cast<V>(atof(columnText));
        }

        V extract(sqlite3_stmt* stmt, int columnIndex, const real_tag&) const {
            return static_cast<V>(sqlite3_column_double(stmt, columnIndex));
        }

        V extract(sqlite3_value* value, const real_tag&) const {
            return static_cast<V>(sqlite3_value_double(value));
        }
    };

    /**
     *  Specialization for std::string.
     */
    template<class T>
    struct row_extractor<T, std::enable_if_t<std::is_base_of<std::string, T>::value>> {
        T extract(const char* columnText) const {
            if(columnText) {
                return columnText;
            } else {
                return {};
            }
        }

        T extract(sqlite3_stmt* stmt, int columnIndex) const {
            if(auto cStr = (const char*)sqlite3_column_text(stmt, columnIndex)) {
                return cStr;
            } else {
                return {};
            }
        }

        T extract(sqlite3_value* value) const {
            if(auto cStr = (const char*)sqlite3_value_text(value)) {
                return cStr;
            } else {
                return {};
            }
        }
    };
#ifndef SQLITE_ORM_OMITS_CODECVT
    /**
     *  Specialization for std::wstring.
     */
    template<>
    struct row_extractor<std::wstring, void> {
        std::wstring extract(const char* columnText) const {
            if(columnText) {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return converter.from_bytes(columnText);
            } else {
                return {};
            }
        }

        std::wstring extract(sqlite3_stmt* stmt, int columnIndex) const {
            auto cStr = (const char*)sqlite3_column_text(stmt, columnIndex);
            if(cStr) {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return converter.from_bytes(cStr);
            } else {
                return {};
            }
        }

        std::wstring extract(sqlite3_value* value) const {
            if(auto cStr = (const wchar_t*)sqlite3_value_text16(value)) {
                return cStr;
            } else {
                return {};
            }
        }
    };
#endif  //  SQLITE_ORM_OMITS_CODECVT

    template<class V>
    struct row_extractor<V, std::enable_if_t<is_std_ptr<V>::value>> {
        using unqualified_type = std::remove_cv_t<typename V::element_type>;

        V extract(const char* columnText) const
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_column_text_extractable<unqualified_type>)
#endif
        {
            if(columnText) {
                const row_extractor<unqualified_type> rowExtractor{};
                return is_std_ptr<V>::make(rowExtractor.extract(columnText));
            } else {
                return {};
            }
        }

        V extract(sqlite3_stmt* stmt, int columnIndex) const
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_row_value_extractable<unqualified_type>)
#endif
        {
            auto type = sqlite3_column_type(stmt, columnIndex);
            if(type != SQLITE_NULL) {
                const row_extractor<unqualified_type> rowExtractor{};
                return is_std_ptr<V>::make(rowExtractor.extract(stmt, columnIndex));
            } else {
                return {};
            }
        }

        V extract(sqlite3_value* value) const
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_boxed_value_extractable<unqualified_type>)
#endif
        {
            auto type = sqlite3_value_type(value);
            if(type != SQLITE_NULL) {
                const row_extractor<unqualified_type> rowExtractor{};
                return is_std_ptr<V>::make(rowExtractor.extract(value));
            } else {
                return {};
            }
        }
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class V>
    struct row_extractor<V, std::enable_if_t<polyfill::is_specialization_of_v<V, std::optional>>> {
        using unqualified_type = std::remove_cv_t<typename V::value_type>;

        V extract(const char* columnText) const
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_column_text_extractable<unqualified_type>)
#endif
        {
            if(columnText) {
                const row_extractor<unqualified_type> rowExtractor{};
                return std::make_optional(rowExtractor.extract(columnText));
            } else {
                return std::nullopt;
            }
        }

        V extract(sqlite3_stmt* stmt, int columnIndex) const
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_row_value_extractable<unqualified_type>)
#endif
        {
            auto type = sqlite3_column_type(stmt, columnIndex);
            if(type != SQLITE_NULL) {
                const row_extractor<unqualified_type> rowExtractor{};
                return std::make_optional(rowExtractor.extract(stmt, columnIndex));
            } else {
                return std::nullopt;
            }
        }

        V extract(sqlite3_value* value) const
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires(orm_boxed_value_extractable<unqualified_type>)
#endif
        {
            auto type = sqlite3_value_type(value);
            if(type != SQLITE_NULL) {
                const row_extractor<unqualified_type> rowExtractor{};
                return std::make_optional(rowExtractor.extract(value));
            } else {
                return std::nullopt;
            }
        }
    };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

    template<>
    struct row_extractor<nullptr_t, void> {
        nullptr_t extract(const char* /*columnText*/) const {
            return nullptr;
        }

        nullptr_t extract(sqlite3_stmt*, int /*columnIndex*/) const {
            return nullptr;
        }

        nullptr_t extract(sqlite3_value*) const {
            return nullptr;
        }
    };
    /**
     *  Specialization for std::vector<char>.
     */
    template<>
    struct row_extractor<std::vector<char>, void> {
        std::vector<char> extract(const char* columnText) const {
            return {columnText, columnText + (columnText ? strlen(columnText) : 0)};
        }

        std::vector<char> extract(sqlite3_stmt* stmt, int columnIndex) const {
            auto bytes = static_cast<const char*>(sqlite3_column_blob(stmt, columnIndex));
            auto len = static_cast<size_t>(sqlite3_column_bytes(stmt, columnIndex));
            return {bytes, bytes + len};
        }

        std::vector<char> extract(sqlite3_value* value) const {
            auto bytes = static_cast<const char*>(sqlite3_value_blob(value));
            auto len = static_cast<size_t>(sqlite3_value_bytes(value));
            return {bytes, bytes + len};
        }
    };

    /**
     *  Specialization for locking_mode.
     */
    template<>
    struct row_extractor<locking_mode, void> {
        locking_mode extract(const char* columnText) const {
            if(columnText) {
                auto resultPair = internal::locking_mode_from_string(columnText);
                if(resultPair.first) {
                    return resultPair.second;
                } else {
                    throw std::system_error{orm_error_code::incorrect_locking_mode_string};
                }
            } else {
                throw std::system_error{orm_error_code::incorrect_locking_mode_string};
            }
        }

        locking_mode extract(sqlite3_stmt* stmt, int columnIndex) const {
            auto cStr = (const char*)sqlite3_column_text(stmt, columnIndex);
            return this->extract(cStr);
        }

        locking_mode extract(sqlite3_value* value) const = delete;
    };

    /**
     *  Specialization for journal_mode.
     */
    template<>
    struct row_extractor<journal_mode, void> {
        journal_mode extract(const char* columnText) const {
            if(columnText) {
                auto resultPair = internal::journal_mode_from_string(columnText);
                if(resultPair.first) {
                    return resultPair.second;
                } else {
                    throw std::system_error{orm_error_code::incorrect_journal_mode_string};
                }
            } else {
                throw std::system_error{orm_error_code::incorrect_journal_mode_string};
            }
        }

        journal_mode extract(sqlite3_stmt* stmt, int columnIndex) const {
            auto cStr = (const char*)sqlite3_column_text(stmt, columnIndex);
            return this->extract(cStr);
        }

        journal_mode extract(sqlite3_value* value) const = delete;
    };

    namespace internal {

        /*
         *  Helper to extract a structure from a rowset.
         */
        template<class R, class DBOs>
        struct struct_extractor;

#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
        /*  
         *  Returns a value-based row extractor for an unmapped type,
         *  returns a structure extractor for a table reference, tuple or named struct.
         */
        template<class R, class DBOs>
        auto make_row_extractor([[maybe_unused]] const DBOs& dbObjects) {
            if constexpr(polyfill::is_specialization_of_v<R, std::tuple> ||
                         polyfill::is_specialization_of_v<R, structure> || is_table_reference_v<R>) {
                return struct_extractor<R, DBOs>{dbObjects};
            } else {
                return row_value_extractor<R>();
            }
        }
#else
        /*  
         *  Overload for an unmapped type returns a common row extractor.
         */
        template<
            class R,
            class DBOs,
            std::enable_if_t<polyfill::negation<polyfill::disjunction<polyfill::is_specialization_of<R, std::tuple>,
                                                                      polyfill::is_specialization_of<R, structure>,
                                                                      is_table_reference<R>>>::value,
                             bool> = true>
        auto make_row_extractor(const DBOs& /*dbObjects*/) {
            return row_value_extractor<R>();
        }

        /*  
         *  Overload for a table reference, tuple or aggregate of column results returns a structure extractor.
         */
        template<class R,
                 class DBOs,
                 std::enable_if_t<polyfill::disjunction<polyfill::is_specialization_of<R, std::tuple>,
                                                        polyfill::is_specialization_of<R, structure>,
                                                        is_table_reference<R>>::value,
                                  bool> = true>
        struct_extractor<R, DBOs> make_row_extractor(const DBOs& dbObjects) {
            return {dbObjects};
        }
#endif

        /**
         *  Specialization for a tuple of top-level column results.
         */
        template<class DBOs, class... Args>
        struct struct_extractor<std::tuple<Args...>, DBOs> {
            const DBOs& db_objects;

            std::tuple<Args...> extract(const char* columnText) const = delete;

            // note: expects to be called only from the top level, and therefore discards the index
            std::tuple<column_result_proxy_t<Args>...> extract(sqlite3_stmt* stmt,
                                                               int&& /*nextColumnIndex*/ = 0) const {
                int columnIndex = -1;
                return {make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
            }

            // unused to date
            std::tuple<column_result_proxy_t<Args>...> extract(sqlite3_stmt* stmt, int& columnIndex) const = delete;

            std::tuple<Args...> extract(sqlite3_value* value) const = delete;
        };

        /**
         *  Specialization for an unmapped structure to be constructed ad-hoc from column results.
         *  
         *  This plays together with `column_result_of_t`, which returns `struct_t<O>` as `structure<O>`
         */
        template<class O, class... Args, class DBOs>
        struct struct_extractor<structure<O, std::tuple<Args...>>, DBOs> {
            const DBOs& db_objects;

            O extract(const char* columnText) const = delete;

            // note: expects to be called only from the top level, and therefore discards the index;
            // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
            //       see unit test tests/prepared_statement_tests/select.cpp/TEST_CASE("Prepared select")/SECTION("non-aggregate struct")
            template<class Ox = O, satisfies<is_eval_order_garanteed, Ox> = true>
            O extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/ = 0) const {
                int columnIndex = -1;
                return O{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
            }

            template<class Ox = O, satisfies_not<is_eval_order_garanteed, Ox> = true>
            O extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/ = 0) const {
                int columnIndex = -1;
                // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
                std::tuple<Args...> t{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
                return create_from_tuple<O>(std::move(t), std::index_sequence_for<Args...>{});
            }

            // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
            //       see unit test tests/prepared_statement_tests/select.cpp/TEST_CASE("Prepared select")/SECTION("non-aggregate struct")
            template<class Ox = O, satisfies<is_eval_order_garanteed, Ox> = true>
            O extract(sqlite3_stmt* stmt, int& columnIndex) const {
                --columnIndex;
                return O{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
            }

            template<class Ox = O, satisfies_not<is_eval_order_garanteed, Ox> = true>
            O extract(sqlite3_stmt* stmt, int& columnIndex) const {
                --columnIndex;
                // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
                std::tuple<Args...> t{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
                return create_from_tuple<O>(std::move(t), std::index_sequence_for<Args...>{});
            }

            O extract(sqlite3_value* value) const = delete;
        };
    }
}
