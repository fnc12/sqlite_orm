#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("flatten_primry_keys_columns") {
    struct User {
        int id = 0;
        std::string name;
    };
    {  //  id + name
        auto pk1 = primary_key(&User::id);
        auto pk2 = primary_key(&User::name);

        using Pk1 = decltype(pk1);
        using Pk2 = decltype(pk2);
        using Result = internal::flatten_primry_keys_columns<Pk1, Pk2>::columns_tuple;
        using Expected = std::tuple<decltype(&User::id), decltype(&User::name)>;
        STATIC_REQUIRE(std::is_same<Result, Expected>::value);
    }
    {  //  (empty) + name
        auto pk1 = primary_key();
        auto pk2 = primary_key(&User::name);

        using Pk1 = decltype(pk1);
        using Pk2 = decltype(pk2);
        using Result = internal::flatten_primry_keys_columns<Pk1, Pk2>::columns_tuple;
        using Expected = std::tuple<decltype(&User::name)>;
        STATIC_REQUIRE(std::is_same<Result, Expected>::value);
    }
    {  //  id + (empty)
        auto pk1 = primary_key(&User::id);
        auto pk2 = primary_key();

        using Pk1 = decltype(pk1);
        using Pk2 = decltype(pk2);
        using Result = internal::flatten_primry_keys_columns<Pk1, Pk2>::columns_tuple;
        using Expected = std::tuple<decltype(&User::id)>;
        STATIC_REQUIRE(std::is_same<Result, Expected>::value);
    }
    {  //  (empty) + (empty)
        auto pk1 = primary_key();
        auto pk2 = primary_key();

        using Pk1 = decltype(pk1);
        using Pk2 = decltype(pk2);
        using Result = internal::flatten_primry_keys_columns<Pk1, Pk2>::columns_tuple;
        using Expected = std::tuple<>;
        STATIC_REQUIRE(std::is_same<Result, Expected>::value);
    }
}
