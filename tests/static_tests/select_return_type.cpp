#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "static_tests_common.h"
#include "static_tests_storage_traits.h"

using namespace sqlite_orm;

TEST_CASE("Select return types") {
    auto storage = make_storage("", make_table("users", make_column("id", &User::id)));
    //  this call is important - it tests compilation in inner storage_t::serialize_column_schema function
    storage.sync_schema();
    {
        using SelectVectorInt = decltype(storage.select(&User::id));
        STATIC_REQUIRE(std::is_same<SelectVectorInt, std::vector<int>>::value);

        using SelectVectorTuple = decltype(storage.select(columns(&User::id)));
        auto ids = storage.select(columns(&User::id));
        STATIC_REQUIRE(std::is_same<decltype(ids), SelectVectorTuple>::value);
        STATIC_REQUIRE(std::is_same<SelectVectorTuple, std::vector<std::tuple<int>>>::value);
        using IdsTuple = SelectVectorTuple::value_type;
        STATIC_REQUIRE(std::tuple_size<IdsTuple>::value == 1);
    }
    //  test storage traits
    {
        using namespace sqlite_orm::internal::storage_traits;
        struct Visit {
            int id = 0;
            std::string date;
        };

        //  test is_mapped
        STATIC_REQUIRE(internal::is_mapped_v<decltype(storage)::db_objects_type, User>);
        STATIC_REQUIRE(!internal::is_mapped_v<decltype(storage)::db_objects_type, Visit>);

        //  test is_storage
        STATIC_REQUIRE(internal::is_storage<decltype(storage)>::value);
        STATIC_REQUIRE(!internal::is_storage<User>::value);
        STATIC_REQUIRE(!internal::is_storage<int>::value);
        STATIC_REQUIRE(!internal::is_storage<void>::value);

        auto storage2 = make_storage(
            "",
            make_table("visits", make_column("id", &Visit::id, primary_key()), make_column("date", &Visit::date)));

        //  test storage_columns_count
        STATIC_REQUIRE(storage_columns_count<decltype(storage), User>::value == 1);
        STATIC_REQUIRE(storage_columns_count<decltype(storage), Visit>::value == 0);
        STATIC_REQUIRE(storage_columns_count<decltype(storage2), Visit>::value == 2);

        //  test storage mapped columns
        using MappedUserColumnsTypes = storage_mapped_columns<decltype(storage)::db_objects_type, User>::type;
        STATIC_REQUIRE(std::is_same<MappedUserColumnsTypes, std::tuple<int>>::value);

        using MappedVisitColumnsEmpty = storage_mapped_columns<decltype(storage)::db_objects_type, Visit>::type;
        STATIC_REQUIRE(std::is_same<MappedVisitColumnsEmpty, std::tuple<>>::value);

        using MappedVisitColumnTypes = storage_mapped_columns<decltype(storage2)::db_objects_type, Visit>::type;
        STATIC_REQUIRE(std::is_same<MappedVisitColumnTypes, std::tuple<int, std::string>>::value);
    }
}
