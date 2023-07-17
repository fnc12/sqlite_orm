#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::enable_if_t, std::is_arithmetic, std::is_same, std::enable_if
#include <stdlib.h>  //  atof, atoi, atoll
#include <system_error>  //  std::system_error
#include <string>  //  std::string, std::wstring
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <codecvt>  //  std::wstring_convert, std::codecvt_utf8_utf16
#endif  //  SQLITE_ORM_OMITS_CODECVT
#include <vector>  //  std::vector
#include <cstring>  //  strlen
#include <locale>
#include <algorithm>  //  std::copy
#include <iterator>  //  std::back_inserter
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element

#include "functional/cxx_universal.h"
#include "arithmetic_tag.h"
#include "pointer_value.h"
#include "journal_mode.h"
#include "error_code.h"
#include "is_std_ptr.h"

namespace sqlite_orm {

    /**
     *  Helper for casting values originating from SQL to C++ typed values, usually from rows of a result set.
     *  
     *  sqlite_orm provides specializations for known C++ types, users may define their custom specialization
     *  of this helper.
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
         *  Called before invocation of a user-defined scalar or aggregate functions,
         *  in order to unpack a boxed "dynamic" function value into a tuple of function arguments.
         */
        V extract(sqlite3_value* value) const = delete;
    };

#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
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

    template<class R>
    int extract_single_value(void* data, int argc, char** argv, char**) {
        auto& res = *(R*)data;
        if(argc) {
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
            static_assert(orm_column_text_extractable<R>);
#endif
            const row_extractor<R> rowExtractor{};
            res = rowExtractor.extract(argv[0]);
        }
        return 0;
    }

    /**
     *  Specialization for the 'pointer-passing interface'.
     * 
     *  @note The 'pointer-passing' interface doesn't support (and in fact prohibits)
     *  extracting pointers from columns.
     */
    template<class P, class T>
    struct row_extractor<pointer_arg<P, T>, void> {
        using V = pointer_arg<P, T>;

        V extract(sqlite3_value* value) const {
            return {(P*)sqlite3_value_pointer(value, T::value)};
        }
    };

    /**
     * Undefine using pointer_binding<> for querying values
     */
    template<class P, class T, class D>
    struct row_extractor<pointer_binding<P, T, D>, void>;

    /**
     *  Specialization for arithmetic types.
     */
    template<class V>
    struct row_extractor<V, std::enable_if_t<std::is_arithmetic<V>::value>> {
        V extract(const char* colTxt) const {
            return this->extract(colTxt, tag());
        }

        V extract(sqlite3_stmt* stmt, int colIdx) const {
            return this->extract(stmt, colIdx, tag());
        }

        V extract(sqlite3_value* value) const {
            return this->extract(value, tag());
        }

      private:
        using tag = arithmetic_tag_t<V>;

        V extract(const char* colTxt, const int_or_smaller_tag&) const {
            return static_cast<V>(atoi(colTxt));
        }

        V extract(sqlite3_stmt* stmt, int colIdx, const int_or_smaller_tag&) const {
            return static_cast<V>(sqlite3_column_int(stmt, colIdx));
        }

        V extract(sqlite3_value* value, const int_or_smaller_tag&) const {
            return static_cast<V>(sqlite3_value_int(value));
        }

        V extract(const char* colTxt, const bigint_tag&) const {
            return static_cast<V>(atoll(colTxt));
        }

        V extract(sqlite3_stmt* stmt, int colIdx, const bigint_tag&) const {
            return static_cast<V>(sqlite3_column_int64(stmt, colIdx));
        }

        V extract(sqlite3_value* value, const bigint_tag&) const {
            return static_cast<V>(sqlite3_value_int64(value));
        }

        V extract(const char* colTxt, const real_tag&) const {
            return static_cast<V>(atof(colTxt));
        }

        V extract(sqlite3_stmt* stmt, int colIdx, const real_tag&) const {
            return static_cast<V>(sqlite3_column_double(stmt, colIdx));
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
        T extract(const char* colTxt) const {
            if(colTxt) {
                return colTxt;
            } else {
                return {};
            }
        }

        T extract(sqlite3_stmt* stmt, int colIdx) const {
            if(auto cStr = (const char*)sqlite3_column_text(stmt, colIdx)) {
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
        std::wstring extract(const char* colTxt) const {
            if(colTxt) {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return converter.from_bytes(colTxt);
            } else {
                return {};
            }
        }

        std::wstring extract(sqlite3_stmt* stmt, int colIdx) const {
            auto cStr = (const char*)sqlite3_column_text(stmt, colIdx);
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

        V extract(const char* colTxt) const
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
            requires(orm_column_text_extractable<unqualified_type>)
#endif
        {
            if(colTxt) {
                const row_extractor<unqualified_type> rowExtractor{};
                return is_std_ptr<V>::make(rowExtractor.extract(colTxt));
            } else {
                return {};
            }
        }

        V extract(sqlite3_stmt* stmt, int colIdx) const
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
            requires(orm_row_value_extractable<unqualified_type>)
#endif
        {
            auto type = sqlite3_column_type(stmt, colIdx);
            if(type != SQLITE_NULL) {
                const row_extractor<unqualified_type> rowExtractor{};
                return is_std_ptr<V>::make(rowExtractor.extract(stmt, colIdx));
            } else {
                return {};
            }
        }

        V extract(sqlite3_value* value) const
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
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

        V extract(const char* colTxt) const
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
            requires(orm_column_text_extractable<unqualified_type>)
#endif
        {
            if(colTxt) {
                const row_extractor<unqualified_type> rowExtractor{};
                return std::make_optional(rowExtractor.extract(colTxt));
            } else {
                return std::nullopt;
            }
        }

        V extract(sqlite3_stmt* stmt, int colIdx) const
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
            requires(orm_row_value_extractable<unqualified_type>)
#endif
        {
            auto type = sqlite3_column_type(stmt, colIdx);
            if(type != SQLITE_NULL) {
                const row_extractor<unqualified_type> rowExtractor{};
                return std::make_optional(rowExtractor.extract(stmt, colIdx));
            } else {
                return std::nullopt;
            }
        }

        V extract(sqlite3_value* value) const
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
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
    struct row_extractor<nullptr_t> {
        nullptr_t extract(const char* /*colTxt*/) const {
            return nullptr;
        }

        nullptr_t extract(sqlite3_stmt*, int /*colIdx*/) const {
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
    struct row_extractor<std::vector<char>> {
        std::vector<char> extract(const char* colTxt) const {
            return {colTxt, colTxt + (colTxt ? ::strlen(colTxt) : 0)};
        }

        std::vector<char> extract(sqlite3_stmt* stmt, int colIdx) const {
            auto bytes = static_cast<const char*>(sqlite3_column_blob(stmt, colIdx));
            auto len = static_cast<size_t>(sqlite3_column_bytes(stmt, colIdx));
            return {bytes, bytes + len};
        }

        std::vector<char> extract(sqlite3_value* value) const {
            auto bytes = static_cast<const char*>(sqlite3_value_blob(value));
            auto len = static_cast<size_t>(sqlite3_value_bytes(value));
            return {bytes, bytes + len};
        }
    };

    /**
     *  Specialization for a tuple.
     */
    template<class... Args>
    struct row_extractor<std::tuple<Args...>> {

        std::tuple<Args...> extract(char** argv) const {
            return this->extract(argv, std::make_index_sequence<sizeof...(Args)>{});
        }

        std::tuple<Args...> extract(sqlite3_stmt* stmt, int /*colIdx*/) const {
            return this->extract(stmt, std::make_index_sequence<sizeof...(Args)>{});
        }

      protected:
        template<size_t... Idx>
        std::tuple<Args...> extract(sqlite3_stmt* stmt, std::index_sequence<Idx...>) const {
            return {row_extractor<Args>{}.extract(stmt, Idx)...};
        }

        template<size_t... Idx>
        std::tuple<Args...> extract(char** argv, std::index_sequence<Idx...>) const {
            return {row_extractor<Args>{}.extract(argv[Idx])...};
        }
    };

    /**
     *  Specialization for journal_mode.
     */
    template<>
    struct row_extractor<journal_mode, void> {
        journal_mode extract(const char* colTxt) const {
            if(colTxt) {
                if(auto res = internal::journal_mode_from_string(colTxt)) {
                    return std::move(*res);
                } else {
                    throw std::system_error{orm_error_code::incorrect_journal_mode_string};
                }
            } else {
                throw std::system_error{orm_error_code::incorrect_journal_mode_string};
            }
        }

        journal_mode extract(sqlite3_stmt* stmt, int colIdx) const {
            auto cStr = (const char*)sqlite3_column_text(stmt, colIdx);
            return this->extract(cStr);
        }
    };
}
