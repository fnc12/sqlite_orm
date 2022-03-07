#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::enable_if_t, std::is_arithmetic, std::is_same, std::true_type, std::false_type
#include <memory>  //  std::default_delete
#include <string>  //  std::string, std::wstring
#include <vector>  //  std::vector
#include <cstddef>  //  std::nullptr_t
#ifndef SQLITE_ORM_STRING_VIEW_SUPPORTED
#include <cstring>  //  ::strncpy, ::strlen
#include <cwchar>  //  ::wcsncpy, ::wcslen
#endif

#include "start_macros.h"
#include "cxx_polyfill.h"
#include "is_std_ptr.h"
#include "arithmetic_tag.h"
#include "xdestroy_handling.h"
#include "pointer_value.h"

namespace sqlite_orm {

    /**
     *  Helper class used for binding fields to sqlite3 statements.
     */
    template<class V, typename Enable = void>
    struct statement_binder;

    /**
     *  Specialization for 'pointer-passing interface'.
     */
    template<class P, class T, class D>
    struct statement_binder<pointer_binding<P, T, D>, void> {
        using V = pointer_binding<P, T, D>;

        // ownership of pointed-to-object is left untouched and remains at prepared statement's AST expression
        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            // note: C-casting `P* -> void*`, internal::xdestroy_proxy() does the inverse
            return sqlite3_bind_pointer(stmt, index, (void*)value.ptr(), T::value, null_xdestroy_f);
        }

        // ownership of pointed-to-object is transferred to sqlite
        void result(sqlite3_context* context, V& value) const {
            // note: C-casting `P* -> void*`,
            // row_extractor<pointer_arg<P, T>>::extract() and internal::xdestroy_proxy() do the inverse
            sqlite3_result_pointer(context, (void*)value.take_ptr(), T::value, value.get_xdestroy());
        }
    };

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
        std::enable_if_t<std::is_base_of<std::string, V>::value || std::is_same<V, const char*>::value
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                         || std::is_same_v<V, std::string_view>
#endif
                         >> {

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            auto stringData = this->string_data(value);
            return sqlite3_bind_text(stmt, index, stringData.first, stringData.second, SQLITE_TRANSIENT);
        }

        void result(sqlite3_context* context, const V& value) const {
            auto stringData = this->string_data(value);
            auto dataCopy = new char[stringData.second + 1];
            constexpr auto deleter = std::default_delete<char[]>{};
            ::strncpy(dataCopy, stringData.first, stringData.second + 1);
            sqlite3_result_text(context, dataCopy, stringData.second, obtain_xdestroy_for(deleter, dataCopy));
        }

      private:
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
        std::pair<const char*, int> string_data(const std::string_view& s) const {
            return {s.data(), int(s.size())};
        }
#else
        std::pair<const char*, int> string_data(const std::string& s) const {
            return {s.c_str(), int(s.size())};
        }

        std::pair<const char*, int> string_data(const char* s) const {
            return {s, int(::strlen(s))};
        }
#endif
    };

#ifndef SQLITE_ORM_OMITS_CODECVT
    /**
     *  Specialization for std::wstring and C-wstring (UTF-16 assumed).
     */
    template<class V>
    struct statement_binder<
        V,
        std::enable_if_t<(std::is_base_of<std::wstring, V>::value || std::is_same<V, const wchar_t*>::value
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                          || std::is_same_v<V, std::wstring_view>
#endif
                          )>> {

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            auto stringData = this->string_data(value);
            return sqlite3_bind_text16(stmt,
                                       index,
                                       stringData.first,
                                       stringData.second * sizeof(wchar_t),
                                       SQLITE_TRANSIENT);
        }

        void result(sqlite3_context* context, const V& value) const {
            auto stringData = this->string_data(value);
            auto dataCopy = new wchar_t[stringData.second + 1];
            constexpr auto deleter = std::default_delete<wchar_t[]>{};
            ::wcsncpy(dataCopy, stringData.first, stringData.second + 1);
            sqlite3_result_text16(context,
                                  dataCopy,
                                  stringData.second * sizeof(wchar_t),
                                  obtain_xdestroy_for(deleter, dataCopy));
        }

      private:
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
        std::pair<const wchar_t*, int> string_data(const std::wstring_view& s) const {
            return {s.data(), int(s.size())};
        }
#else
        std::pair<const wchar_t*, int> string_data(const std::wstring& s) const {
            return {s.c_str(), int(s.size())};
        }

        std::pair<const wchar_t*, int> string_data(const wchar_t* s) const {
            return {s, int(::wcslen(s))};
        }
#endif
    };
#endif

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
     *  Specialization for binary data (std::vector<char>).
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

        template<class T, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_bindable_v = false;
        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_bindable_v<T, polyfill::void_t<decltype(statement_binder<T>{})>> = true;
        template<class T>
        using is_bindable = polyfill::bool_constant<is_bindable_v<T>>;

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
