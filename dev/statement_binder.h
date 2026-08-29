#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_arithmetic, std::is_same, std::make_index_sequence, std::index_sequence
#include <memory>  //  std::default_delete
#include <string>  //  std::string, std::wstring
#include <string_view>  //  std::string_view
#include <vector>  //  std::vector
#include <cstring>  //  strncpy
#include <functional>  //  std::invoke
#include <optional>  //  std::optional
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <locale>  // std::wstring_convert
#include <codecvt>  //  std::codecvt_utf8_utf16
#endif
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "vocabulary/algorithms/field_predicates_fwd.h"  // Included to define is_bindable_v
#include "type_traits.h"
#include "is_std_ptr.h"
#include "tuple_helper/tuple_filter.h"
#include "error_code.h"
#include "arithmetic_tag.h"
#include "xdestroy_handling.h"
#include "pointer_value.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Helper class used for binding fields to sqlite3 statements.
     */
    template<class V, typename Enable = void>
    struct statement_binder;
}

namespace sqlite_orm::internal {
    /*
     *  Implementation note: the technique of indirect expression testing is because
     *  of older compilers having problems with the detection of dependent templates [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE].
     *  It must also be a type that differs from those for `is_printable_v`, `is_preparable_statement_v`.
     */
    template<class Binder>
    struct indirectly_test_bindable;

    template<class T, class SFINAE>
    constexpr bool is_bindable_v = false;
    template<class T>
    constexpr bool is_bindable_v<T, std::void_t<indirectly_test_bindable<decltype(statement_binder<T>{})>>> = true;
}

// `statement_binder` specializations;
// note: no need to export the specializations, only the primary template above
namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3020000
    /**
     *  Specialization for pointer bindings (part of the 'pointer-passing interface').
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
#endif

    /**
     *  Specialization for arithmetic types.
     */
    template<class V>
    struct statement_binder<V, internal::match_if<std::is_arithmetic, V>> {

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
                            std::enable_if_t<std::disjunction<std::is_base_of<std::string, V>,
                                                              std::is_same<V, orm_gsl::czstring>,
                                                              std::is_same<V, std::string_view>>::value>> {

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            auto stringData = this->string_data(value);
            return sqlite3_bind_text(stmt, index, stringData.first, stringData.second, SQLITE_TRANSIENT);
        }

        void result(sqlite3_context* context, const V& value) const {
            auto stringData = this->string_data(value);
            auto dataCopy = new char[stringData.second + 1];
            constexpr auto deleter = std::default_delete<char[]>{};
            strncpy(dataCopy, stringData.first, stringData.second + 1);
            sqlite3_result_text(context, dataCopy, stringData.second, obtain_xdestroy_for(deleter, dataCopy));
        }

      private:
        std::pair<const char*, int> string_data(const std::string_view& s) const {
            return {s.data(), int(s.size())};
        }
    };

#ifndef SQLITE_ORM_OMITS_CODECVT
    template<class V>
    struct statement_binder<V,
                            std::enable_if_t<std::disjunction<std::is_base_of<std::wstring, V>,
                                                              std::is_same<V, const wchar_t*>,
                                                              std::is_same<V, std::wstring_view>>::value>> {

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
        std::pair<const wchar_t*, int> string_data(const std::wstring_view& s) const {
            return {s.data(), int(s.size())};
        }
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

    template<class V>
    struct statement_binder<
        V,
        std::enable_if_t<is_std_ptr<V>::value &&
                         internal::is_bindable<std::remove_cv_t<typename V::element_type>>::value>> {
        using unqualified_type = std::remove_cv_t<typename V::element_type>;

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            if (value) {
                return statement_binder<unqualified_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<std::nullptr_t>().bind(stmt, index, nullptr);
            }
        }
    };

    /**
     *  Specialization for binary data (std::vector<char>).
     */
    template<>
    struct statement_binder<std::vector<char>, void> {
        int bind(sqlite3_stmt* stmt, int index, const std::vector<char>& value) const {
            if (!value.empty()) {
                return sqlite3_bind_blob(stmt, index, value.data(), int(value.size()), SQLITE_TRANSIENT);
            } else {
                return sqlite3_bind_blob(stmt, index, "", 0, SQLITE_TRANSIENT);
            }
        }

        void result(sqlite3_context* context, const std::vector<char>& value) const {
            if (!value.empty()) {
                sqlite3_result_blob(context, value.data(), int(value.size()), nullptr);
            } else {
                sqlite3_result_blob(context, "", 0, nullptr);
            }
        }
    };

    template<class V>
    struct statement_binder<V,
                            std::enable_if_t<polyfill::is_specialization_of_v<V, std::optional> &&
                                             internal::is_bindable_v<std::remove_cv_t<typename V::value_type>>>> {
        using unqualified_type = std::remove_cv_t<typename V::value_type>;

        int bind(sqlite3_stmt* stmt, int index, const V& value) const {
            if (value) {
                return statement_binder<unqualified_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<std::nullopt_t>().bind(stmt, index, std::nullopt);
            }
        }
    };
}

namespace sqlite_orm::internal {
    struct conditional_binder {
        sqlite3_stmt* stmt = nullptr;
        int nthSqlParameter = 0;

        explicit conditional_binder(sqlite3_stmt* stmt) : stmt{stmt} {}

        template<class T, satisfies<is_bindable, T> = true>
        void operator()(const T& t) {
            const int rc = statement_binder<T>{}.bind(this->stmt, ++this->nthSqlParameter, t);
            if (SQLITE_OK != rc) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                throw_translated_sqlite_error(rc);
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
            if (!value) {
                throw std::system_error{orm_error_code::value_is_null};
            }
            (*this)(*value);
        }
    };

    struct tuple_value_binder {
        sqlite3_stmt* stmt = nullptr;

        explicit tuple_value_binder(sqlite3_stmt* stmt) : stmt{stmt} {}

#ifdef SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
        template<class Tpl, class Projection>
        void operator()(const Tpl& tpl, Projection project) const {
            int nthSqlParameter = 0;
            auto& [... elements] = tpl;
            (this->bind(std::invoke(project, elements), ++nthSqlParameter), ...);
        }
#else
        template<class Tpl, class Projection>
        void operator()(const Tpl& tpl, Projection project) const {
            (*this)(tpl, std::make_index_sequence<std::tuple_size<Tpl>::value>{}, std::forward<Projection>(project));
        }
#endif

      private:
#ifndef SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
        template<class Tpl, size_t... Idx, class Projection>
        void operator()(const Tpl& tpl, std::index_sequence<Idx...>, Projection project) const {
            (this->bind(std::invoke(project, std::get<Idx>(tpl)), int(Idx + 1)), ...);
        }
#endif

        template<class T>
        void bind(const T& t, int nthSqlParameter) const {
            const int rc = statement_binder<T>{}.bind(this->stmt, nthSqlParameter, t);
            if (SQLITE_OK != rc) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                throw_translated_sqlite_error(rc);
            }
        }

        template<class T>
        void bind(const T* value, int nthSqlParameter) const {
            if (!value) {
                throw std::system_error{orm_error_code::value_is_null};
            }
            this->bind(*value, nthSqlParameter);
        }
    };

    template<class Tpl>
    using bindable_filter_t = filter_tuple_t<Tpl, is_bindable>;
}
