#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::enable_if_t, std::is_arithmetic, std::is_same, std::true_type, std::false_type, std::make_index_sequence, std::index_sequence
#include <memory>  //  std::default_delete
#include <string>  //  std::string, std::wstring
#include <vector>  //  std::vector
#include <cstring>  //  ::strncpy, ::strlen
#include "functional/cxx_string_view.h"
#ifndef SQLITE_ORM_STRING_VIEW_SUPPORTED
#include <cwchar>  //  ::wcsncpy, ::wcslen
#endif

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "functional/cxx_functional_polyfill.h"
#include "is_std_ptr.h"
#include "tuple_helper/tuple_filter.h"
#include "error_code.h"
#include "arithmetic_tag.h"
#include "xdestroy_handling.h"
#include "pointer_value.h"

namespace sqlite_orm {

    /**
     *  Helper class used for binding fields to sqlite3 statements.
     */
    template<class V, typename Enable = void>
    struct statement_binder;

    namespace internal {

        template<class T, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_bindable_v = false;
        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_bindable_v<T, polyfill::void_t<decltype(statement_binder<T>())>> = true
            // note : msvc 14.0 needs the parentheses constructor, otherwise `is_bindable<const char*>` isn't recognised.
            // The strangest thing is that this is mutually exclusive with `is_printable_v`.
            ;

        template<class T>
        using is_bindable = polyfill::bool_constant<is_bindable_v<T>>;

    }

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

        int bind(sqlite3_stmt* stmt, int index, const V& value, int_or_smaller_tag) const {
            return sqlite3_bind_int(stmt, index, static_cast<int>(value));
        }

        void result(sqlite3_context* context, const V& value, int_or_smaller_tag) const {
            sqlite3_result_int(context, static_cast<int>(value));
        }

        int bind(sqlite3_stmt* stmt, int index, const V& value, bigint_tag) const {
            return sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
        }

        void result(sqlite3_context* context, const V& value, bigint_tag) const {
            sqlite3_result_int64(context, static_cast<sqlite3_int64>(value));
        }

        int bind(sqlite3_stmt* stmt, int index, const V& value, real_tag) const {
            return sqlite3_bind_double(stmt, index, static_cast<double>(value));
        }

        void result(sqlite3_context* context, const V& value, real_tag) const {
            sqlite3_result_double(context, static_cast<double>(value));
        }
    };

    /**
     *  Specialization for std::string and C-string.
     */
    template<class V>
    struct statement_binder<V,
                            std::enable_if_t<polyfill::disjunction_v<std::is_base_of<std::string, V>,
                                                                     std::is_same<V, const char*>
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                                                                     ,
                                                                     std::is_same<V, std::string_view>
#endif
                                                                     >>> {

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
    template<class V>
    struct statement_binder<V,
                            std::enable_if_t<polyfill::disjunction_v<std::is_base_of<std::wstring, V>,
                                                                     std::is_same<V, const wchar_t*>
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                                                                     ,
                                                                     std::is_same<V, std::wstring_view>
#endif
                                                                     >>> {

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            auto stringData = this->string_data(value);
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string utf8Str = converter.to_bytes(stringData.first, stringData.first + stringData.second);
            return statement_binder<decltype(utf8Str)>().bind(stmt, index, utf8Str);
        }

        void result(sqlite3_context* context, const V& value) const {
            auto stringData = this->string_data(value);
            sqlite3_result_text16(context, stringData.first, stringData.second, nullptr);
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
     *  Specialization for nullptr_t.
     */
    template<>
    struct statement_binder<nullptr_t, void> {
        int bind(sqlite3_stmt* stmt, int index, const nullptr_t&) const {
            return sqlite3_bind_null(stmt, index);
        }

        void result(sqlite3_context* context, const nullptr_t&) const {
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
    struct statement_binder<
        V,
        std::enable_if_t<is_std_ptr<V>::value && internal::is_bindable_v<std::remove_cv_t<typename V::element_type>>>> {
        using unqualified_type = std::remove_cv_t<typename V::element_type>;

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            if(value) {
                return statement_binder<unqualified_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<nullptr_t>().bind(stmt, index, nullptr);
            }
        }
    };

    /**
     *  Specialization for binary data (std::vector<char>).
     */
    template<>
    struct statement_binder<std::vector<char>, void> {
        int bind(sqlite3_stmt* stmt, int index, const std::vector<char>& value) const {
            if(!value.empty()) {
                return sqlite3_bind_blob(stmt, index, (const void*)&value.front(), int(value.size()), SQLITE_TRANSIENT);
            } else {
                return sqlite3_bind_blob(stmt, index, "", 0, SQLITE_TRANSIENT);
            }
        }

        void result(sqlite3_context* context, const std::vector<char>& value) const {
            if(!value.empty()) {
                sqlite3_result_blob(context, (const void*)&value.front(), int(value.size()), nullptr);
            } else {
                sqlite3_result_blob(context, "", 0, nullptr);
            }
        }
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class V>
    struct statement_binder<V,
                            std::enable_if_t<polyfill::is_specialization_of_v<V, std::optional> &&
                                             internal::is_bindable_v<std::remove_cv_t<typename V::value_type>>>> {
        using unqualified_type = std::remove_cv_t<typename V::value_type>;

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            if(value) {
                return statement_binder<unqualified_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<std::nullopt_t>().bind(stmt, index, std::nullopt);
            }
        }
    };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

    namespace internal {

        struct conditional_binder {
            sqlite3_stmt* stmt = nullptr;
            int index = 1;

            explicit conditional_binder(sqlite3_stmt* stmt) : stmt{stmt} {}

            template<class T, satisfies<is_bindable, T> = true>
            void operator()(const T& t) {
                int rc = statement_binder<T>{}.bind(this->stmt, this->index++, t);
                if(SQLITE_OK != rc) {
                    throw_translated_sqlite_error(this->stmt);
                }
            }

            template<class T, satisfies_not<is_bindable, T> = true>
            void operator()(const T&) const {}
        };

        struct field_value_binder : conditional_binder {
            using conditional_binder::conditional_binder;
            using conditional_binder::operator();

            template<class T, satisfies_not<is_bindable, T> = true>
            void operator()(const T&) const = delete;

            template<class T>
            void operator()(const T* value) {
                if(!value) {
                    throw std::system_error{orm_error_code::value_is_null};
                }
                (*this)(*value);
            }
        };

        struct tuple_value_binder {
            sqlite3_stmt* stmt = nullptr;

            explicit tuple_value_binder(sqlite3_stmt* stmt) : stmt{stmt} {}

            template<class Tpl, class Projection>
            void operator()(const Tpl& tpl, Projection project) const {
                (*this)(tpl,
                        std::make_index_sequence<std::tuple_size<Tpl>::value>{},
                        std::forward<Projection>(project));
            }

          private:
#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
            template<class Tpl, size_t... Idx, class Projection>
            void operator()(const Tpl& tpl, std::index_sequence<Idx...>, Projection project) const {
                (this->bind(polyfill::invoke(project, std::get<Idx>(tpl)), Idx), ...);
            }
#else
            template<class Tpl, size_t I, size_t... Idx, class Projection>
            void operator()(const Tpl& tpl, std::index_sequence<I, Idx...>, Projection project) const {
                this->bind(polyfill::invoke(project, std::get<I>(tpl)), I);
                (*this)(tpl, std::index_sequence<Idx...>{}, std::forward<Projection>(project));
            }

            template<class Tpl, class Projection>
            void operator()(const Tpl&, std::index_sequence<>, Projection) const {}
#endif

            template<class T>
            void bind(const T& t, size_t idx) const {
                int rc = statement_binder<T>{}.bind(this->stmt, int(idx + 1), t);
                if(SQLITE_OK != rc) {
                    throw_translated_sqlite_error(this->stmt);
                }
            }

            template<class T>
            void bind(const T* value, size_t idx) const {
                if(!value) {
                    throw std::system_error{orm_error_code::value_is_null};
                }
                (*this)(*value, idx);
            }
        };

        template<class Tpl>
        using bindable_filter_t = filter_tuple_t<Tpl, is_bindable>;
    }
}
