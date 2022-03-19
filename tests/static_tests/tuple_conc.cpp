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
        using IntStringTuple = conc_tuple<TupleL, TupleR>::type;
        STATIC_REQUIRE(std::is_same<IntStringTuple, std::tuple<int, std::string>>::value);
    }
    {
        using TupleL = std::tuple<int>;
        using TupleR = std::tuple<float>;
        using IntFloatTuple = conc_tuple<TupleL, TupleR>::type;
        STATIC_REQUIRE(std::is_same<IntFloatTuple, std::tuple<int, float>>::value);
    }
    {
        using TupleL = std::tuple<>;
        using TupleR = std::tuple<float>;
        using NoneFloatTuple = conc_tuple<TupleL, TupleR>::type;
        STATIC_REQUIRE(std::is_same<NoneFloatTuple, std::tuple<float>>::value);
    }
    {
        using TupleL = std::tuple<>;
        using TupleR = std::tuple<>;
        using NoneNoneTuple = conc_tuple<TupleL, TupleR>::type;
        STATIC_REQUIRE(std::is_same<NoneNoneTuple, std::tuple<>>::value);
    }
    {
        using TupleL = std::tuple<int, float>;
        using TupleR = std::tuple<double>;
        using IntFloatDoubleTuple = conc_tuple<TupleL, TupleR>::type;
        STATIC_REQUIRE(std::is_same<IntFloatDoubleTuple, std::tuple<int, float, double>>::value);
    }
    {
        using Arg = std::tuple<int>;
        using SingleArgTuple = conc_tuple<Arg>::type;
        STATIC_REQUIRE(std::is_same<Arg, SingleArgTuple>::value);
    }
    {
        using Arg1 = std::tuple<int>;
        using Arg2 = std::tuple<float>;
        using Arg3 = std::tuple<std::string>;
        using IntFloatStringTuple = conc_tuple<Arg1, Arg2, Arg3>::type;
        STATIC_REQUIRE(std::is_same<IntFloatStringTuple, std::tuple<int, float, std::string>>::value);
    }
    {
        using Arg1 = std::tuple<int>;
        using Arg2 = std::tuple<float>;
        using Arg3 = std::tuple<>;
        using IntFloatEmptyTuple = conc_tuple<Arg1, Arg2, Arg3>::type;
        STATIC_REQUIRE(std::is_same<IntFloatEmptyTuple, std::tuple<int, float>>::value);
    }
}
