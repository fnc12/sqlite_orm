#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "static_tests_common.h"

using namespace sqlite_orm;

TEST_CASE("Select return types") {
    auto storage = make_storage("", make_table("users", make_column("id", &User::id)));
    //  this call is important - it tests compilation in inner storage_t::serialize_column_schema function
    storage.sync_schema();
    {
        using SelectVectorInt = decltype(storage.select(&User::id));
        static_assert(std::is_same<SelectVectorInt, std::vector<int>>::value, "Incorrect select id vector type");

        using SelectVectorTuple = decltype(storage.select(columns(&User::id)));
        auto ids = storage.select(columns(&User::id));
        static_assert(std::is_same<decltype(ids), SelectVectorTuple>::value, "");
        static_assert(std::is_same<SelectVectorTuple, std::vector<std::tuple<int>>>::value,
                      "Incorrect select id vector type");
        using IdsTuple = SelectVectorTuple::value_type;
        static_assert(std::tuple_size<IdsTuple>::value == 1, "Incorrect tuple size");
    }
    {
        //  test storage traits
        struct Visit {
            int id = 0;
            std::string date;
        };
        using namespace sqlite_orm::internal::storage_traits;

        //  test type_is_mapped
        static_assert(type_is_mapped<decltype(storage), User>::value, "User must be mapped to a storage");
        static_assert(!type_is_mapped<decltype(storage), Visit>::value, "User must be mapped to a storage");

        //  test is_storage
        static_assert(internal::is_storage<decltype(storage)>::value, "is_storage works incorrectly");
        static_assert(!internal::is_storage<User>::value, "is_storage works incorrectly");
        static_assert(!internal::is_storage<int>::value, "is_storage works incorrectly");
        static_assert(!internal::is_storage<void>::value, "is_storage works incorrectly");

        auto storage2 = make_storage(
            "",
            make_table("visits", make_column("id", &Visit::id, primary_key()), make_column("date", &Visit::date)));

        //  test storage_columns_count
        static_assert(storage_columns_count<decltype(storage), User>::value == 1,
                      "Incorrect storage columns count value");
        static_assert(storage_columns_count<decltype(storage), Visit>::value == 0,
                      "Incorrect storage columns count value");
        static_assert(storage_columns_count<decltype(storage2), Visit>::value == 2,
                      "Incorrect storage columns count value");

        //  test storage mapped columns
        using UserColumnsTuple = storage_mapped_columns<decltype(storage), User>::type;
        static_assert(std::is_same<UserColumnsTuple, std::tuple<int>>::value,
                      "Incorrect storage_mapped_columns result");

        using VisitColumsEmptyType = storage_mapped_columns<decltype(storage), Visit>::type;
        static_assert(std::is_same<VisitColumsEmptyType, std::tuple<>>::value,
                      "Incorrect storage_mapped_columns result");

        using VisitColumnTypes = storage_mapped_columns<decltype(storage2), Visit>::type;
        static_assert(std::is_same<VisitColumnTypes, std::tuple<int, std::string>>::value,
                      "Incorrect storage_mapped_columns result");
    }
}
