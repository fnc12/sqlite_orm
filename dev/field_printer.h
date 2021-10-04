#pragma once

#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <vector>  //  std::vector
#include <cstddef>  //  std::nullptr_t
#include <memory>  //  std::shared_ptr, std::unique_ptr
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

namespace sqlite_orm {

    /**
     *  Is used to print members mapped to objects in storage_t::dump member function.
     *  Other developers can create own specialization to map custom types
     */
    template<class T, typename Enable = void>
    struct field_printer {
        std::string operator()(const T& t) const {
            std::stringstream stream;
            stream << t;
            return stream.str();
        }
    };

    /**
     *  Upgrade to integer is required when using unsigned char(uint8_t)
     */
    template<>
    struct field_printer<unsigned char, void> {
        std::string operator()(const unsigned char& t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };

    /**
     *  Upgrade to integer is required when using signed char(int8_t)
     */
    template<>
    struct field_printer<signed char, void> {
        std::string operator()(const signed char& t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };

    /**
     *  char is neigher signer char nor unsigned char so it has its own specialization
     */
    template<>
    struct field_printer<char, void> {
        std::string operator()(const char& t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };

    template<>
    struct field_printer<std::string, void> {
        std::string operator()(const std::string& t) const {
            return t;
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

    template<>
    struct field_printer<std::nullptr_t, void> {
        std::string operator()(const std::nullptr_t&) const {
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
    struct field_printer<std::shared_ptr<T>, void> {
        std::string operator()(const std::shared_ptr<T>& t) const {
            if(t) {
                return field_printer<T>()(*t);
            } else {
                return field_printer<std::nullptr_t>()(nullptr);
            }
        }
    };

    template<class T>
    struct field_printer<std::unique_ptr<T>, void> {
        std::string operator()(const std::unique_ptr<T>& t) const {
            if(t) {
                return field_printer<T>()(*t);
            } else {
                return field_printer<std::nullptr_t>()(nullptr);
            }
        }
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    struct field_printer<std::optional<T>, void> {
        std::string operator()(const std::optional<T>& t) const {
            if(t.has_value()) {
                return field_printer<T>()(*t);
            } else {
                return field_printer<std::nullptr_t>()(nullptr);
            }
        }
    };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
}
