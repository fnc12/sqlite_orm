#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <memory>  //  std::unique_ptr, std::shared_ptr
#include <string>  //  std::string
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  //  std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
#include <string_view>
#endif

using namespace sqlite_orm;
using internal::is_printable_v;

struct Custom {};
template<class Elem>
class StringVeneer : public std::basic_string<Elem> {};

namespace sqlite_orm {
    template<>
    struct field_printer<Custom> {};
}

TEST_CASE("is_printable") {
    struct User {
        int id;
    };
    static_assert(is_printable_v<bool>, "bool must be printable");
    static_assert(is_printable_v<char>, "char must be printable");
    static_assert(is_printable_v<signed char>, "char must be printable");
    static_assert(is_printable_v<unsigned char>, "char must be printable");
    static_assert(is_printable_v<short>, "short must be printable");
    static_assert(is_printable_v<unsigned short>, "short must be printable");
    static_assert(is_printable_v<int>, "int must be printable");
    static_assert(is_printable_v<unsigned int>, "int must be printable");
    static_assert(is_printable_v<long>, "long must be printable");
    static_assert(is_printable_v<unsigned long>, "long must be printable");
    static_assert(is_printable_v<float>, "float must be printable");
    static_assert(is_printable_v<long long>, "long long must be printable");
    static_assert(is_printable_v<unsigned long long>, "long long must be printable");
    static_assert(is_printable_v<double>, "double must be printable");
    static_assert(!is_printable_v<const char*>, "C string must not be printable");
    static_assert(is_printable_v<std::string>, "string must be printable");
    static_assert(is_printable_v<StringVeneer<char>>, "Derived string must be printable");
#ifndef SQLITE_ORM_OMITS_CODECVT
    static_assert(!is_printable_v<const wchar_t*>, "wide C string must not be printable");
    static_assert(is_printable_v<std::wstring>, "wstring must be printable");
    static_assert(is_printable_v<StringVeneer<wchar_t>>, "Derived wstring must be printable");
#endif
    static_assert(is_printable_v<std::nullptr_t>, "null must be printable");
    static_assert(is_printable_v<std::unique_ptr<int>>, "unique_ptr must be printable");
    static_assert(is_printable_v<std::shared_ptr<int>>, "shared_ptr must be printable");
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
    static_assert(!is_printable_v<std::string_view>, "string view must not be printable");
    static_assert(!is_printable_v<std::wstring_view>, "wstring view must not be printable");
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    static_assert(is_printable_v<std::nullopt_t>, "nullopt must be printable");
    static_assert(is_printable_v<std::optional<int>>, "optional must be printable");
    static_assert(is_printable_v<std::optional<Custom>>, "optional<Custom> must be printable");
    static_assert(!is_printable_v<std::optional<User>>, "optional<User> cannot be printable");
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
    static_assert(!is_printable_v<static_pointer_binding<nullptr_t, carray_pvt>>,
                  "pointer binding must not be printable");

    static_assert(is_printable_v<Custom>, "Custom must be printable");
    static_assert(is_printable_v<std::unique_ptr<Custom>>, "unique_ptr<Custom> must be printable");

    static_assert(!is_printable_v<void>, "void cannot be printable");
    static_assert(!is_printable_v<User>, "User cannot be printable");
    static_assert(!is_printable_v<std::unique_ptr<User>>, "unique_ptr<User> cannot be printable");
}
