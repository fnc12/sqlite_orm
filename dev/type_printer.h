#pragma once

#include <string>  //  std::string
#include <memory>  //  std::shared_ptr, std::unique_ptr
#include <vector>  //  std::vector
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

namespace sqlite_orm {

    /**
     *  This class accepts c++ type and transfers it to sqlite name (int -> INTEGER, std::string -> TEXT)
     */
    template<class T, typename Enable = void>
    struct type_printer;

    struct integer_printer {
        inline const std::string& print() {
            static const std::string res = "INTEGER";
            return res;
        }
    };

    struct text_printer {
        inline const std::string& print() {
            static const std::string res = "TEXT";
            return res;
        }
    };

    struct real_printer {
        inline const std::string& print() {
            static const std::string res = "REAL";
            return res;
        }
    };

    struct blob_printer {
        inline const std::string& print() {
            static const std::string res = "BLOB";
            return res;
        }
    };

    // Note unsigned/signed char and simple char used for storing integer values, not char values.
    template<>
    struct type_printer<unsigned char, void> : public integer_printer {};

    template<>
    struct type_printer<signed char, void> : public integer_printer {};

    template<>
    struct type_printer<char, void> : public integer_printer {};

    template<>
    struct type_printer<unsigned short int, void> : public integer_printer {};

    template<>
    struct type_printer<short, void> : public integer_printer {};

    template<>
    struct type_printer<unsigned int, void> : public integer_printer {};

    template<>
    struct type_printer<int, void> : public integer_printer {};

    template<>
    struct type_printer<unsigned long, void> : public integer_printer {};

    template<>
    struct type_printer<long, void> : public integer_printer {};

    template<>
    struct type_printer<unsigned long long, void> : public integer_printer {};

    template<>
    struct type_printer<long long, void> : public integer_printer {};

    template<>
    struct type_printer<bool, void> : public integer_printer {};

    template<>
    struct type_printer<std::string, void> : public text_printer {};

    template<>
    struct type_printer<std::wstring, void> : public text_printer {};

    template<>
    struct type_printer<const char*, void> : public text_printer {};

    template<>
    struct type_printer<float, void> : public real_printer {};

    template<>
    struct type_printer<double, void> : public real_printer {};

    template<class T>
    struct type_printer<std::shared_ptr<T>, void> : public type_printer<T> {};

    template<class T>
    struct type_printer<std::unique_ptr<T>, void> : public type_printer<T> {};

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    struct type_printer<std::optional<T>, void> : public type_printer<T> {};
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

    template<>
    struct type_printer<std::vector<char>, void> : public blob_printer {};
}
