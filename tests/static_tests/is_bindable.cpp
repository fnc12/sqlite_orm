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
using internal::is_bindable_v;

struct Custom {};
template<class Elem>
class StringVeneer : public std::basic_string<Elem> {};

namespace sqlite_orm {
    template<>
    struct statement_binder<Custom> {};
    template<>
    struct field_printer<Custom> {};
}

TEST_CASE("is_bindable") {
    struct User {
        int id;
    };
    static_assert(is_bindable_v<bool>, "bool must be bindable");
    static_assert(is_bindable_v<char>, "char must be bindable");
    static_assert(is_bindable_v<signed char>, "char must be bindable");
    static_assert(is_bindable_v<unsigned char>, "char must be bindable");
    static_assert(is_bindable_v<short>, "short must be bindable");
    static_assert(is_bindable_v<unsigned short>, "short must be bindable");
    static_assert(is_bindable_v<int>, "int must be bindable");
    static_assert(is_bindable_v<unsigned int>, "int must be bindable");
    static_assert(is_bindable_v<long>, "long must be bindable");
    static_assert(is_bindable_v<unsigned long>, "long must be bindable");
    static_assert(is_bindable_v<float>, "float must be bindable");
    static_assert(is_bindable_v<long long>, "long long must be bindable");
    static_assert(is_bindable_v<unsigned long long>, "long long must be bindable");
    static_assert(is_bindable_v<double>, "double must be bindable");
    static_assert(is_bindable_v<const char*>, "C string must be bindable");
    static_assert(is_bindable_v<std::string>, "string must be bindable");
    static_assert(is_bindable_v<StringVeneer<char>>, "Derived string must be bindable");
#ifndef SQLITE_ORM_OMITS_CODECVT
    static_assert(is_bindable_v<const wchar_t*>, "wide C string must be bindable");
    static_assert(is_bindable_v<std::wstring>, "wstring must be bindable");
    static_assert(is_bindable_v<StringVeneer<wchar_t>>, "Derived wstring must be bindable");
#endif
    static_assert(is_bindable_v<std::nullptr_t>, "null must be bindable");
    static_assert(is_bindable_v<std::unique_ptr<int>>, "unique_ptr must be bindable");
    static_assert(is_bindable_v<std::shared_ptr<int>>, "shared_ptr must be bindable");
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
    static_assert(is_bindable_v<std::string_view>, "string view must be bindable");
#ifndef SQLITE_ORM_OMITS_CODECVT
    static_assert(is_bindable_v<std::wstring_view>, "wstring view must be bindable");
#endif
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    static_assert(is_bindable_v<std::nullopt_t>, "nullopt must be bindable");
    static_assert(is_bindable_v<std::optional<int>>, "optional must be bindable");
    static_assert(is_bindable_v<std::optional<Custom>>, "optional<Custom> must be bindable");
    static_assert(!is_bindable_v<std::optional<User>>, "optional<User> cannot be bindable");
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
    static_assert(is_bindable_v<static_pointer_binding<nullptr_t, carray_pvt>>, "pointer binding must be bindable");

    static_assert(is_bindable_v<Custom>, "Custom must be bindable");
    static_assert(is_bindable_v<std::unique_ptr<Custom>>, "unique_ptr<Custom> must be bindable");

    static_assert(!is_bindable_v<void>, "void cannot be bindable");
    static_assert(!is_bindable_v<User>, "User cannot be bindable");
    static_assert(!is_bindable_v<std::unique_ptr<User>>, "unique_ptr<User> cannot be bindable");
    {
        auto isEqual = is_equal(&User::id, 5);
        static_assert(!is_bindable_v<decltype(isEqual)>, "is_equal cannot be bindable");
    }
    {
        auto notEqual = is_not_equal(&User::id, 10);
        static_assert(!is_bindable_v<decltype(notEqual)>, "is_not_equal cannot be bindable");
    }
    {
        auto lesserThan = lesser_than(&User::id, 10);
        static_assert(!is_bindable_v<decltype(lesserThan)>, "lesser_than cannot be bindable");
    }
    {
        auto lesserOrEqual = lesser_or_equal(&User::id, 5);
        static_assert(!is_bindable_v<decltype(lesserOrEqual)>, "lesser_or_equal cannot be bindable");
    }
    {
        auto greaterThan = greater_than(&User::id, 5);
        static_assert(!is_bindable_v<decltype(greaterThan)>, "greater_than cannot be bindable");
    }
    {
        auto greaterOrEqual = greater_or_equal(&User::id, 5);
        static_assert(!is_bindable_v<decltype(greaterOrEqual)>, "greater_or_equal cannot be bindable");
    }
    {
        auto func = datetime("now");
        static_assert(!is_bindable_v<decltype(func)>, "datetime cannot be bindable");
        bool trueCalled = false;
        bool falseCalled = false;
        auto dummy = 5;  //  for gcc compilation
        internal::static_if<is_bindable_v<decltype(func)>>(
            [&trueCalled](int&) {
                trueCalled = true;
            },
            [&falseCalled](int&) {
                falseCalled = true;
            })(dummy);
        REQUIRE(!trueCalled);
        REQUIRE(falseCalled);
    }
}
