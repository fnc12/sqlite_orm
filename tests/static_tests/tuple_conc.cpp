#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <string>  //  std::string
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("Tuple conc") {
    using namespace internal;
    {
        using TupleL = std::tuple<int>;
        using TupleR = std::tuple<std::string>;
        using ConcRes = conc_tuple<TupleL, TupleR>::type;
        static_assert(std::is_same<ConcRes, std::tuple<int, std::string>>::value, "int + string didn't work");
    }
    {
        using TupleL = std::tuple<int>;
        using TupleR = std::tuple<float>;
        using ConcRes = conc_tuple<TupleL, TupleR>::type;
        static_assert(std::is_same<ConcRes, std::tuple<int, float>>::value, "int + float didn't work");
    }
    {
        using TupleL = std::tuple<>;
        using TupleR = std::tuple<float>;
        using ConcRes = conc_tuple<TupleL, TupleR>::type;
        static_assert(std::is_same<ConcRes, std::tuple<float>>::value, "none + float didn't work");
    }
    {
        using TupleL = std::tuple<>;
        using TupleR = std::tuple<>;
        using ConcRes = conc_tuple<TupleL, TupleR>::type;
        static_assert(std::is_same<ConcRes, std::tuple<>>::value, "none + none didn't work");
    }
    {
        using TupleL = std::tuple<int, float>;
        using TupleR = std::tuple<double>;
        using ConcRes = conc_tuple<TupleL, TupleR>::type;
        static_assert(std::is_same<ConcRes, std::tuple<int, float, double>>::value, "int, float + double didn't work");
    }
    {
        using Arg = std::tuple<int>;
        using ConcRes = conc_tuple<Arg>::type;
        static_assert(std::is_same<Arg, ConcRes>::value, "Single argument is incorrect");
    }
    {
        using Arg1 = std::tuple<int>;
        using Arg2 = std::tuple<float>;
        using Arg3 = std::tuple<std::string>;
        using ConcRes = conc_tuple<Arg1, Arg2, Arg3>::type;
        using Expected = std::tuple<int, float, std::string>;
        static_assert(std::is_same<Expected, ConcRes>::value, "int + float + std::string");
    }
    {
        using Arg1 = std::tuple<int>;
        using Arg2 = std::tuple<float>;
        using Arg3 = std::tuple<>;
        using ConcRes = conc_tuple<Arg1, Arg2, Arg3>::type;
        using Expected = std::tuple<int, float>;
        static_assert(std::is_same<Expected, ConcRes>::value, "int + float + (empty)");
    }
}
