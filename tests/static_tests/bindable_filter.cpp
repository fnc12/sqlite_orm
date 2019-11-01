#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("bindable_filter") {
    struct User {
        int id = 0;
        std::string name;
    };

    using internal::bindable_filter;
    using std::is_same;
    {
        using Tuple = std::tuple<bool,
                                 char,
                                 unsigned char,
                                 signed char,
                                 short,
                                 int,
                                 unsigned int,
                                 long,
                                 unsigned long,
                                 long long,
                                 unsigned long long,
                                 float,
                                 double,
                                 long double>;
        using Res = bindable_filter<Tuple>::type;
        static_assert(is_same<Res, Tuple>::value, "");
    }
    {
        using Tuple = std::tuple<decltype(&User::id), decltype(&User::name), int>;
        using Res = bindable_filter<Tuple>::type;
        using Expected = std::tuple<int>;
        static_assert(is_same<Res, Expected>::value, "");
    }
    {
        using Tuple = std::tuple<std::string, decltype(&User::name), float, decltype(&User::id), short>;
        using Res = bindable_filter<Tuple>::type;
        using Expected = std::tuple<std::string, float, short>;
        static_assert(is_same<Res, Expected>::value, "");
    }
}
