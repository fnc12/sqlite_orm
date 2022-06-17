#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <string>  //  std::string
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;
using mpl::tuple;

TEST_CASE("Tuple conc") {
    using namespace internal;
    {
        using TupleL = tuple<int>;
        using TupleR = tuple<std::string>;
        using IntStringTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<IntStringTuple, tuple<int, std::string>>::value);
    }
    {
        using TupleL = tuple<int>;
        using TupleR = tuple<float>;
        using IntFloatTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<IntFloatTuple, tuple<int, float>>::value);
    }
    {
        using TupleL = tuple<>;
        using TupleR = tuple<float>;
        using NoneFloatTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<NoneFloatTuple, tuple<float>>::value);
    }
    {
        using TupleL = tuple<>;
        using TupleR = tuple<>;
        using NoneNoneTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<NoneNoneTuple, tuple<>>::value);
    }
    {
        using TupleL = tuple<int, float>;
        using TupleR = tuple<double>;
        using IntFloatDoubleTuple = tuple_cat_t<TupleL, TupleR>;
        STATIC_REQUIRE(std::is_same<IntFloatDoubleTuple, tuple<int, float, double>>::value);
    }
    {
        using Arg = tuple<int>;
        using SingleArgTuple = tuple_cat_t<Arg>;
        STATIC_REQUIRE(std::is_same<Arg, SingleArgTuple>::value);
    }
    {
        using Arg1 = tuple<int>;
        using Arg2 = tuple<float>;
        using Arg3 = tuple<std::string>;
        using IntFloatStringTuple = tuple_cat_t<Arg1, Arg2, Arg3>;
        STATIC_REQUIRE(std::is_same<IntFloatStringTuple, tuple<int, float, std::string>>::value);
    }
    {
        using Arg1 = tuple<int>;
        using Arg2 = tuple<float>;
        using Arg3 = tuple<>;
        using IntFloatEmptyTuple = tuple_cat_t<Arg1, Arg2, Arg3>;
        STATIC_REQUIRE(std::is_same<IntFloatEmptyTuple, tuple<int, float>>::value);
    }
}
