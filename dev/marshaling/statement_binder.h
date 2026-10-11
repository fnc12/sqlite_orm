#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_arithmetic, std::is_same
#include <memory>  //  std::default_delete
#include <string>  //  std::string, std::wstring
#include <string_view>  //  std::string_view
#include <vector>  //  std::vector
#include <cstring>  //  strncpy
#include <optional>  //  std::optional
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <locale>  // std::wstring_convert
#include <codecvt>  //  std::codecvt_utf8_utf16
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/type_traits.h"
#include "../type_mapping/is_std_ptr.h"
#include "../vocabulary/algorithms/field_predicates_fwd.h"  // Included to define is_bindable_v
#include "../type_mapping/arithmetic_tag.h"
#include "../sqlite3/sqlite3_xdestroy.h"
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
    template<class V>
    struct statement_binder<V, internal::match_if<internal::is_pointer_binding, V>> {
        using tag = typename V::tag;

        // ownership of pointed-to-object is left untouched and remains at prepared statement's AST expression
        SQLITE_ORM_STATIC_CALLOP int bind(sqlite3_stmt* stmt, int index, const V& value) SQLITE_ORM_OR_CONST_CALLOP {
            // note: C-casting `P* -> void*`, internal::xdestroy_proxy() does the inverse
            return sqlite3_bind_pointer(stmt, index, (void*)value.ptr(), tag::value, null_xdestroy_f);
        }

        // ownership of pointed-to-object is transferred to sqlite
        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context* context, V& value) SQLITE_ORM_OR_CONST_CALLOP {
            // note: C-casting `P* -> void*`,
            // the `row_extractor` of `pointer_arg` and internal::xdestroy_proxy() do the inverse
            sqlite3_result_pointer(context, (void*)value.take_ptr(), tag::value, value.get_xdestroy());
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

        SQLITE_ORM_STATIC_CALLOP int bind(sqlite3_stmt* stmt, int index, const V& value) SQLITE_ORM_OR_CONST_CALLOP {
            const std::string_view stringData = value;
            return sqlite3_bind_text(stmt, index, stringData.data(), int(stringData.size()), SQLITE_TRANSIENT);
        }

        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context* context, const V& value) SQLITE_ORM_OR_CONST_CALLOP {
            const std::string_view stringData = value;
            auto dataCopy = new char[stringData.size() + 1];
            constexpr auto deleter = std::default_delete<char[]>{};
            strncpy(dataCopy, stringData.data(), stringData.size() + 1);
            sqlite3_result_text(context, dataCopy, int(stringData.size()), obtain_xdestroy_for(deleter, dataCopy));
        }
    };

#ifndef SQLITE_ORM_OMITS_CODECVT
    template<class V>
    struct statement_binder<V,
                            std::enable_if_t<std::disjunction<std::is_base_of<std::wstring, V>,
                                                              std::is_same<V, orm_gsl::cwzstring>,
                                                              std::is_same<V, std::wstring_view>>::value>> {

        SQLITE_ORM_STATIC_CALLOP int bind(sqlite3_stmt* stmt, int index, const V& value) SQLITE_ORM_OR_CONST_CALLOP {
            const std::wstring_view stringData = value;
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string utf8Str = converter.to_bytes(stringData.data(), stringData.data() + stringData.size());
            return statement_binder<decltype(utf8Str)>().bind(stmt, index, utf8Str);
        }

        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context* context, const V& value) SQLITE_ORM_OR_CONST_CALLOP {
            const std::wstring_view stringData = value;
            sqlite3_result_text16(context, stringData.data(), int(stringData.size()), nullptr);
        }
    };
#endif

    /**
     *  Specialization for std::nullptr_t.
     */
    template<>
    struct statement_binder<std::nullptr_t, void> {
        SQLITE_ORM_STATIC_CALLOP int
        bind(sqlite3_stmt* stmt, int index, const std::nullptr_t&) SQLITE_ORM_OR_CONST_CALLOP {
            return sqlite3_bind_null(stmt, index);
        }

        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context* context,
                                             const std::nullptr_t&) SQLITE_ORM_OR_CONST_CALLOP {
            sqlite3_result_null(context);
        }
    };

    /**
     *  Specialization for std::nullopt_t.
     */
    template<>
    struct statement_binder<std::nullopt_t, void> {
        SQLITE_ORM_STATIC_CALLOP int
        bind(sqlite3_stmt* stmt, int index, const std::nullopt_t&) SQLITE_ORM_OR_CONST_CALLOP {
            return sqlite3_bind_null(stmt, index);
        }

        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context* context,
                                             const std::nullopt_t&) SQLITE_ORM_OR_CONST_CALLOP {
            sqlite3_result_null(context);
        }
    };

    template<class V>
    struct statement_binder<
        V,
        std::enable_if_t<is_std_ptr<V>::value &&
                         internal::is_bindable<std::remove_cv_t<typename V::element_type>>::value>> {
        using unqualified_type = std::remove_cv_t<typename V::element_type>;

        SQLITE_ORM_STATIC_CALLOP int bind(sqlite3_stmt* stmt, int index, const V& value) SQLITE_ORM_OR_CONST_CALLOP {
            if (value) {
                return statement_binder<unqualified_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<std::nullptr_t>().bind(stmt, index, nullptr);
            }
        }

        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context*, const V&) SQLITE_ORM_OR_CONST_CALLOP = delete;
    };

    /**
     *  Specialization for binary data (std::vector<char>).
     */
    template<>
    struct statement_binder<std::vector<char>, void> {
        SQLITE_ORM_STATIC_CALLOP int
        bind(sqlite3_stmt* stmt, int index, const std::vector<char>& value) SQLITE_ORM_OR_CONST_CALLOP {
            if (!value.empty()) {
                return sqlite3_bind_blob(stmt, index, value.data(), int(value.size()), SQLITE_TRANSIENT);
            } else {
                return sqlite3_bind_blob(stmt, index, "", 0, SQLITE_TRANSIENT);
            }
        }

        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context* context,
                                             const std::vector<char>& value) SQLITE_ORM_OR_CONST_CALLOP {
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

        SQLITE_ORM_STATIC_CALLOP int bind(sqlite3_stmt* stmt, int index, const V& value) SQLITE_ORM_OR_CONST_CALLOP {
            if (value) {
                return statement_binder<unqualified_type>().bind(stmt, index, *value);
            } else {
                return statement_binder<std::nullopt_t>().bind(stmt, index, std::nullopt);
            }
        }

        SQLITE_ORM_STATIC_CALLOP void result(sqlite3_context*, const V&) SQLITE_ORM_OR_CONST_CALLOP = delete;
    };
}
