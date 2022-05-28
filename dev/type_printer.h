#pragma once

#include <string>  //  std::string
#include <memory>  //  std::shared_ptr, std::unique_ptr
#include <vector>  //  std::vector
#include "functional/cxx_optional.h"

#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "is_std_ptr.h"

namespace sqlite_orm {

    /**
     *  This class transforms a C++ type to a sqlite type name (int -> INTEGER, ...)
     */
    template<class T, typename Enable = void>
    struct type_printer {};

    struct integer_printer {
        const std::string& print() const {
            static const std::string res = "INTEGER";
            return res;
        }
    };

    struct text_printer {
        const std::string& print() const {
            static const std::string res = "TEXT";
            return res;
        }
    };

    struct real_printer {
        const std::string& print() const {
            static const std::string res = "REAL";
            return res;
        }
    };

    struct blob_printer {
        const std::string& print() const {
            static const std::string res = "BLOB";
            return res;
        }
    };

    // Note: char, unsigned/signed char are used for storing integer values, not char values.
    template<class T>
    struct type_printer<T,
                        std::enable_if_t<polyfill::conjunction_v<polyfill::negation<internal::is_any_of<T,
                                                                                                        wchar_t,
#ifdef __cpp_char8_t
                                                                                                        char8_t,
#endif
                                                                                                        char16_t,
                                                                                                        char32_t>>,
                                                                 std::is_integral<T>>>> : integer_printer {
    };

    template<class T>
    struct type_printer<T, std::enable_if_t<std::is_floating_point<T>::value>> : real_printer {};

    template<class T>
    struct type_printer<T,
                        std::enable_if_t<polyfill::disjunction_v<std::is_same<T, const char*>,
                                                                 std::is_base_of<std::string, T>,
                                                                 std::is_base_of<std::wstring, T>>>> : text_printer {};

    template<class T>
    struct type_printer<T, std::enable_if_t<is_std_ptr<T>::value>> : type_printer<typename T::element_type> {};

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    struct type_printer<T, std::enable_if_t<polyfill::is_specialization_of_v<T, std::optional>>>
        : type_printer<typename T::value_type> {};
#endif

    template<>
    struct type_printer<std::vector<char>, void> : blob_printer {};
}
