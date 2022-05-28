#pragma once

#include <locale>  // std::wstring_convert
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <vector>  //  std::vector
#include <memory>  //  std::shared_ptr, std::unique_ptr
#ifndef SQLITE_ORM_OMITS_CODECVT
#include <codecvt>  //  std::codecvt_utf8_utf16
#endif  //  SQLITE_ORM_OMITS_CODECVT
#include "functional/cxx_optional.h"

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "is_std_ptr.h"

namespace sqlite_orm {

    /**
     *  Is used to print members mapped to objects in storage_t::dump member function.
     *  Other developers can create own specialization to map custom types
     */
    template<class T, typename SFINAE = void>
    struct field_printer;

    namespace internal {
        template<class T, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_printable_v = false;
        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_printable_v<T, polyfill::void_t<decltype(field_printer<T>{})>> = true
            // Also see implementation note for `is_bindable_v`
            ;
        template<class T>
        using is_printable = polyfill::bool_constant<is_printable_v<T>>;
    }

    template<class T>
    struct field_printer<T, std::enable_if_t<std::is_arithmetic<T>::value>> {
        std::string operator()(const T& t) const {
            std::stringstream ss;
            ss << t;
            return ss.str();
        }
    };

    /**
     *  Upgrade to integer is required when using unsigned char(uint8_t)
     */
    template<>
    struct field_printer<unsigned char, void> {
        std::string operator()(const unsigned char& t) const {
            std::stringstream ss;
            ss << +t;
            return ss.str();
        }
    };

    /**
     *  Upgrade to integer is required when using signed char(int8_t)
     */
    template<>
    struct field_printer<signed char, void> {
        std::string operator()(const signed char& t) const {
            std::stringstream ss;
            ss << +t;
            return ss.str();
        }
    };

    /**
     *  char is neither signed char nor unsigned char so it has its own specialization
     */
    template<>
    struct field_printer<char, void> {
        std::string operator()(const char& t) const {
            std::stringstream ss;
            ss << +t;
            return ss.str();
        }
    };

    template<class T>
    struct field_printer<T, std::enable_if_t<std::is_base_of<std::string, T>::value>> {
        std::string operator()(std::string string) const {
            return string;
        }
    };

    template<>
    struct field_printer<std::vector<char>, void> {
        std::string operator()(const std::vector<char>& t) const {
            std::stringstream ss;
            ss << std::hex;
            for(auto c: t) {
                ss << c;
            }
            return ss.str();
        }
    };
#ifndef SQLITE_ORM_OMITS_CODECVT
    /**
     *  Specialization for std::wstring (UTF-16 assumed).
     */
    template<class T>
    struct field_printer<T, std::enable_if_t<std::is_base_of<std::wstring, T>::value>> {
        std::string operator()(const std::wstring& wideString) const {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.to_bytes(wideString);
        }
    };
#endif  //  SQLITE_ORM_OMITS_CODECVT
    template<>
    struct field_printer<nullptr_t, void> {
        std::string operator()(const nullptr_t&) const {
            return "null";
        }
    };
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<>
    struct field_printer<std::nullopt_t, void> {
        std::string operator()(const std::nullopt_t&) const {
            return "null";
        }
    };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    struct field_printer<
        T,
        std::enable_if_t<polyfill::conjunction_v<is_std_ptr<T>,
                                                 internal::is_printable<std::remove_cv_t<typename T::element_type>>>>> {
        using unqualified_type = std::remove_cv_t<typename T::element_type>;

        std::string operator()(const T& t) const {
            if(t) {
                return field_printer<unqualified_type>()(*t);
            } else {
                return field_printer<nullptr_t>{}(nullptr);
            }
        }
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    struct field_printer<
        T,
        std::enable_if_t<polyfill::conjunction_v<polyfill::is_specialization_of<T, std::optional>,
                                                 internal::is_printable<std::remove_cv_t<typename T::value_type>>>>> {
        using unqualified_type = std::remove_cv_t<typename T::value_type>;

        std::string operator()(const T& t) const {
            if(t.has_value()) {
                return field_printer<unqualified_type>()(*t);
            } else {
                return field_printer<std::nullopt_t>{}(std::nullopt);
            }
        }
    };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
}
