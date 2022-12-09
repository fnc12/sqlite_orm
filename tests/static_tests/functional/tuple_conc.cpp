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
        using IntStringTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<IntStringTuple, std::tuple<int, std::string>>::value);
    }
    {
        using TupleL = std::tuple<int>;
        using TupleR = std::tuple<float>;
        using IntFloatTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<IntFloatTuple, std::tuple<int, float>>::value);
    }
    {
        using TupleL = std::tuple<>;
        using TupleR = std::tuple<float>;
        using NoneFloatTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<NoneFloatTuple, std::tuple<float>>::value);
    }
    {
        using TupleL = std::tuple<>;
        using TupleR = std::tuple<>;
        using NoneNoneTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<NoneNoneTuple, std::tuple<>>::value);
    }
    {
        using TupleL = std::tuple<int, float>;
        using TupleR = std::tuple<double>;
        using IntFloatDoubleTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<IntFloatDoubleTuple, std::tuple<int, float, double>>::value);
    }
    {
        using Arg = std::tuple<int>;
        using SingleArgTuple = tuple_cat_t<Arg>;
        STATIC_REQUIRE(std::is_same<Arg, SingleArgTuple>::value);
    }
    {
        using Arg1 = std::tuple<int>;
        using Arg2 = std::tuple<float>;
        using Arg3 = std::tuple<std::string>;
        using IntFloatStringTuple = tuple_cat_t<Arg1, Arg2, Arg3>;
        STATIC_REQUIRE(std::is_same<IntFloatStringTuple, std::tuple<int, float, std::string>>::value);
    }
    {
        using Arg1 = std::tuple<int>;
        using Arg2 = std::tuple<float>;
        using Arg3 = std::tuple<>;
        using IntFloatEmptyTuple = tuple_cat_t<Arg1, Arg2, Arg3>;
        STATIC_REQUIRE(std::is_same<IntFloatEmptyTuple, std::tuple<int, float>>::value);
    }
}
