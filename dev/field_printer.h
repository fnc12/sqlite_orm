#pragma once

#include <type_traits>  //  std::enable_if_t, std::is_arithmetic, std::is_same, std::enable_if
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <vector>  //  std::vector
#include <cstddef>  //  std::nullptr_t
#include <memory>  //  std::shared_ptr, std::unique_ptr

#include "is_std_ptr.h"
namespace sqlite_orm {

    /**
     *  Is used to print members mapped to objects in storage_t::dump member function.
     *  Other developers can create own specialization to map custom types
     */
    template<class T, typename Enable = void>
    struct field_printer {
        std::string operator()(const T &t) const {
            std::stringstream stream;
            stream << t;
            return stream.str();
        }
    };

    /**
     *  Upgrade to integer is required when using unsigned char(uint8_t)
     */
    template<>
    struct field_printer<unsigned char> {
        std::string operator()(const unsigned char &t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };

    /**
     *  Upgrade to integer is required when using signed char(int8_t)
     */
    template<>
    struct field_printer<signed char> {
        std::string operator()(const signed char &t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };

    /**
     *  char is neigher signer char nor unsigned char so it has its own specialization
     */
    template<>
    struct field_printer<char> {
        std::string operator()(const char &t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };

    template<>
    struct field_printer<std::string> {
        std::string operator()(const std::string &t) const {
            return t;
        }
    };

    template<>
    struct field_printer<std::vector<char>> {
        std::string operator()(const std::vector<char> &t) const {
            std::stringstream ss;
            ss << std::hex;
            for(auto c: t) {
                ss << c;
            }
            return ss.str();
        }
    };

    template<>
    struct field_printer<std::nullptr_t> {
        std::string operator()(const std::nullptr_t &) const {
            return "null";
        }
    };

    template<class T>
    struct field_printer<T, std::enable_if_t<is_std_ptr<T>::value>> {
        using container_type = typename is_std_ptr<T>::container_type;

        std::string operator()(const container_type &t) const {
            if(is_std_ptr<T>::isEmpty(t)) {
                return field_printer<T>()(*t);
            } else {
                return field_printer<std::nullptr_t>()(nullptr);
            }
        }
    };
}
