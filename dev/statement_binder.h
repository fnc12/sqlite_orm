#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::enable_if_t, std::is_arithmetic, std::is_same, std::true_type, std::false_type
#include <string>  //  std::string, std::wstring
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <codecvt>  //  std::codecvt_utf8_utf16
#endif  //  SQLITE_ORM_OMITS_CODECVT
#include <vector>  //  std::vector
#include <cstddef>  //  std::nullptr_t
#include <utility>  //  std::declval
#include <locale>  //  std::wstring_convert
#include <cstring>  //  ::strncpy, ::strlen

#include "is_std_ptr.h"

namespace sqlite_orm {

    /**
     *  Helper class used for binding fields to sqlite3 statements.
     */
    template<class V, typename Enable = void>
    struct statement_binder : std::false_type {};

    /**
     *  Specialization for arithmetic types.
     */
    template<class V>
    struct statement_binder<V, std::enable_if_t<std::is_arithmetic<V>::value>> {

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            return this->bind(stmt, index, value, tag());
        }

        void result(sqlite3_context* context, const V& value) const {
            this->result(context, value, tag());
        }

      private:
        using tag = arithmetic_tag_t<V>;

        int bind(sqlite3_stmt* stmt, int index, const V& value, const int_or_smaller_tag&) const {
            return sqlite3_bind_int(stmt, index, static_cast<int>(value));
        }

        void result(sqlite3_context* context, const V& value, const int_or_smaller_tag&) const {
            sqlite3_result_int(context, static_cast<int>(value));
        }

        int bind(sqlite3_stmt* stmt, int index, const V& value, const bigint_tag&) const {
            return sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
        }

        void result(sqlite3_context* context, const V& value, const bigint_tag&) const {
            sqlite3_result_int64(context, static_cast<sqlite3_int64>(value));
        }

        int bind(sqlite3_stmt* stmt, int index, const V& value, const real_tag&) const {
            return sqlite3_bind_double(stmt, index, static_cast<double>(value));
        }

        void result(sqlite3_context* context, const V& value, const real_tag&) const {
            sqlite3_result_double(context, static_cast<double>(value));
        }
    };

    /**
     *  Specialization for std::string and C-string.
     */
    template<class V>
    struct statement_binder<
        V,
        std::enable_if_t<std::is_same<V, std::string>::value || std::is_same<V, const char*>::value>> {

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            auto stringData = this->string_data(value);
            return sqlite3_bind_text(stmt, index, std::get<0>(stringData), std::get<1>(stringData), SQLITE_TRANSIENT);
        }

        void result(sqlite3_context* context, const V& value) const {
            auto stringData = this->string_data(value);
            auto stringDataLength = std::get<1>(stringData);
            auto dataCopy = new char[stringDataLength + 1];
            auto stringChars = std::get<0>(stringData);
            ::strncpy(dataCopy, stringChars, stringDataLength + 1);
            sqlite3_result_text(context, dataCopy, stringDataLength, [](void* pointer) {
                auto charPointer = (char*)pointer;
                delete[] charPointer;
            });
        }

      private:
        std::tuple<const char*, int> string_data(const std::string& s) const {
            return {s.c_str(), int(s.size())};
        }

        std::tuple<const char*, int> string_data(const char* s) const {
            auto length = int(::strlen(s));
            return {s, length};
        }
    };

#ifndef SQLITE_ORM_OMITS_CODECVT
    /**
     *  Specialization for std::wstring and C-wstring.
     */
    template<class V>
    struct statement_binder<
        V,
        std::enable_if_t<std::is_same<V, std::wstring>::value || std::is_same<V, const wchar_t*>::value>> {

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string utf8Str = converter.to_bytes(value);
            return statement_binder<decltype(utf8Str)>().bind(stmt, index, utf8Str);
        }

        void result(sqlite3_context* context, const V& value) const {
            sqlite3_result_text16(context, (const void*)value.data(), int(value.length()), nullptr);
        }
    };
#endif  //  SQLITE_ORM_OMITS_CODECVT

    /**
     *  Specialization for std::nullptr_t.
     */
    template<>
    struct statement_binder<std::nullptr_t, void> {
        int bind(sqlite3_stmt* stmt, int index, const std::nullptr_t&) const {
            return sqlite3_bind_null(stmt, index);
        }

        void result(sqlite3_context* context, const std::nullptr_t&) const {
            sqlite3_result_null(context);
        }
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    /**
     *  Specialization for std::nullopt_t.
     */
    template<>
    struct statement_binder<std::nullopt_t, void> {
        int bind(sqlite3_stmt* stmt, int index, const std::nullopt_t&) const {
            return sqlite3_bind_null(stmt, index);
        }

        void result(sqlite3_context* context, const std::nullopt_t&) const {
            sqlite3_result_null(context);
        }
    };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

    template<class V>
    struct statement_binder<V, std::enable_if_t<is_std_ptr<V>::value>> {
        using value_type = typename is_std_ptr<V>::element_type;

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            if(value) {
                return statement_binder<value_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<std::nullptr_t>().bind(stmt, index, nullptr);
            }
        }

        void result(sqlite3_context* context, const V& value) const {
            if(value) {
                statement_binder<value_type>().result(context, value);
            } else {
                statement_binder<std::nullptr_t>().result(context, nullptr);
            }
        }
    };

    /**
     *  Specialization for optional type (std::vector<char>).
     */
    template<>
    struct statement_binder<std::vector<char>, void> {
        int bind(sqlite3_stmt* stmt, int index, const std::vector<char>& value) const {
            if(value.size()) {
                return sqlite3_bind_blob(stmt, index, (const void*)&value.front(), int(value.size()), SQLITE_TRANSIENT);
            } else {
                return sqlite3_bind_blob(stmt, index, "", 0, SQLITE_TRANSIENT);
            }
        }

        void result(sqlite3_context* context, const std::vector<char>& value) const {
            if(value.size()) {
                sqlite3_result_blob(context, (const void*)&value.front(), int(value.size()), nullptr);
            } else {
                sqlite3_result_blob(context, "", 0, nullptr);
            }
        }
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    struct statement_binder<std::optional<T>, void> {
        using value_type = T;

        int bind(sqlite3_stmt* stmt, int index, const std::optional<T>& value) const {
            if(value) {
                return statement_binder<value_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<std::nullopt_t>().bind(stmt, index, std::nullopt);
            }
        }

        void result(sqlite3_context* context, const std::optional<T>& value) const {
            if(value) {
                statement_binder<value_type>().result(context, value);
            } else {
                statement_binder<std::nullopt_t>().result(context, std::nullopt);
            }
        }
    };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

    namespace internal {

        template<class T>
        using is_bindable = std::integral_constant<bool, !std::is_base_of<std::false_type, statement_binder<T>>::value>;

        struct conditional_binder_base {
            sqlite3_stmt* stmt = nullptr;
            int& index;

            conditional_binder_base(decltype(stmt) stmt_, decltype(index) index_) : stmt(stmt_), index(index_) {}
        };

        template<class T, class C>
        struct conditional_binder;

        template<class T>
        struct conditional_binder<T, std::true_type> : conditional_binder_base {

            using conditional_binder_base::conditional_binder_base;

            int operator()(const T& t) const {
                return statement_binder<T>().bind(this->stmt, this->index++, t);
            }
        };

        template<class T>
        struct conditional_binder<T, std::false_type> : conditional_binder_base {
            using conditional_binder_base::conditional_binder_base;

            int operator()(const T&) const {
                return SQLITE_OK;
            }
        };

        template<class T, template<class C> class F, class SFINAE = void>
        struct tuple_filter_single;

        template<class T, template<class C> class F>
        struct tuple_filter_single<T, F, typename std::enable_if<F<T>::value>::type> {
            using type = std::tuple<T>;
        };

        template<class T, template<class C> class F>
        struct tuple_filter_single<T, F, typename std::enable_if<!F<T>::value>::type> {
            using type = std::tuple<>;
        };

        template<class T, template<class C> class F>
        struct tuple_filter;

        template<class... Args, template<class C> class F>
        struct tuple_filter<std::tuple<Args...>, F> {
            using type = typename conc_tuple<typename tuple_filter_single<Args, F>::type...>::type;
        };

        template<class T>
        struct bindable_filter_single : tuple_filter_single<T, is_bindable> {};

        template<class T>
        struct bindable_filter;

        template<class... Args>
        struct bindable_filter<std::tuple<Args...>> {
            using type = typename conc_tuple<typename bindable_filter_single<Args>::type...>::type;
        };
    }
}
