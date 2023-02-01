#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>
#endif

using namespace sqlite_orm;

namespace {
    struct Custom {};
    template<class Elem>
    class StringVeneer : public std::basic_string<Elem> {};
}

namespace sqlite_orm {
    template<>
    struct statement_binder<Custom> {};
}

TEST_CASE("bindable_filter") {
    struct User {
        int id = 0;
        std::string name;
    };

    using internal::bindable_filter_t;
    using std::is_same;
    {
        using Tuple = std::tuple<bool,
                                 char,
                                 unsigned char,
                                 signed char,
                                 short,
                                 unsigned short,
                                 int,
                                 unsigned int,
                                 long,
                                 unsigned long,
                                 long long,
                                 unsigned long long,
                                 float,
                                 double,
                                 long double,
                                 const char*,
                                 std::string,
                                 StringVeneer<char>,
#ifndef SQLITE_ORM_OMITS_CODECVT
                                 const wchar_t*,
                                 std::wstring,
                                 StringVeneer<wchar_t>,
#endif
                                 std::vector<char>,
                                 std::nullptr_t,
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                                 std::nullopt_t,
                                 std::optional<int>,
                                 std::optional<Custom>,
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                                 std::string_view,
#ifndef SQLITE_ORM_OMITS_CODECVT
                                 std::wstring_view,
#endif
#endif
                                 std::unique_ptr<int>,
                                 std::shared_ptr<int>,
#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
                                 static_pointer_binding<std::nullptr_t, carray_pvt>,
#endif
                                 Custom,
                                 std::unique_ptr<Custom>>;
        using Res = bindable_filter_t<Tuple>;
        STATIC_REQUIRE(is_same<Res, Tuple>::value);
    }
    {
        using Tuple = std::tuple<decltype(&User::id), decltype(&User::name), int>;
        using Res = bindable_filter_t<Tuple>;
        using Expected = std::tuple<int>;
        STATIC_REQUIRE(is_same<Res, Expected>::value);
    }
    {
        using Tuple = std::tuple<std::string, decltype(&User::name), float, decltype(&User::id), short>;
        using Res = bindable_filter_t<Tuple>;
        using Expected = std::tuple<std::string, float, short>;
        STATIC_REQUIRE(is_same<Res, Expected>::value);
    }
}
