#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include <type_traits>  //  std::is_same

#include "../static_tests/static_tests_storage_traits.h"

using namespace sqlite_orm;

TEST_CASE("Foreign key") {

    struct Location {
        int id;
        std::string place;
        std::string country;
        std::string city;
        int distance;
    };

    struct Visit {
        int id;
        std::unique_ptr<int> location;
        std::unique_ptr<int> user;
        int visited_at;
        uint8_t mark;
    };

    //  this case didn't compile on linux until `typedef constraints_type` was added to `foreign_key_t`
    auto storage = make_storage("test_fk.sqlite",
                                make_table("location",
                                           make_column("id", &Location::id, primary_key()),
                                           make_column("place", &Location::place),
                                           make_column("country", &Location::country),
                                           make_column("city", &Location::city),
                                           make_column("distance", &Location::distance)),
                                make_table("visit",
                                           make_column("id", &Visit::id, primary_key()),
                                           make_column("location", &Visit::location),
                                           make_column("user", &Visit::user),
                                           make_column("visited_at", &Visit::visited_at),
                                           make_column("mark", &Visit::mark),
                                           foreign_key(&Visit::location).references(&Location::id)));
    {
        using namespace sqlite_orm::internal::storage_traits;

        using Storage = decltype(storage);
        STATIC_REQUIRE(storage_foreign_keys_count<Storage, Location>::value == 1);
        STATIC_REQUIRE(storage_foreign_keys_count<Storage, Visit>::value == 0);

        using LocationFks = storage_fk_references<Storage, Location>::type;
        STATIC_REQUIRE(std::is_same<LocationFks, std::tuple<Visit>>::value);

        using VisitFks = storage_fk_references<Storage, Visit>::type;
        STATIC_REQUIRE(std::is_same<VisitFks, std::tuple<>>::value);
    }
    storage.sync_schema();

    int fromDate = int(std::time(nullptr));
    int toDate = int(std::time(nullptr));
    int toDistance = 100;
    auto id = 10;
    storage.select(columns(&Visit::mark, &Visit::visited_at, &Location::place),
                   inner_join<Location>(on(is_equal(&Visit::location, &Location::id))),
                   where(is_equal(&Visit::user, id) and greater_than(&Visit::visited_at, fromDate) and
                         lesser_than(&Visit::visited_at, toDate) and lesser_than(&Location::distance, toDistance)),
                   order_by(&Visit::visited_at));
}

//  appeared after #57
TEST_CASE("Foreign key 2") {
    class test1 {
      public:
        // Constructors
        test1(){};

        // Variables
        int id;
        std::string val1;
        std::string val2;
    };

    class test2 {
      public:
        // Constructors
        test2(){};

        // Variables
        int id;
        int fk_id;
        std::string val1;
        std::string val2;
    };

    auto table1 = make_table("test_1",
                             make_column("id", &test1::id, primary_key()),
                             make_column("val1", &test1::val1),
                             make_column("val2", &test1::val2));

    auto table2 = make_table("test_2",
                             make_column("id", &test2::id, primary_key()),
                             make_column("fk_id", &test2::fk_id),
                             make_column("val1", &test2::val1),
                             make_column("val2", &test2::val2),
                             foreign_key(&test2::fk_id).references(&test1::id));

    auto storage = make_storage("test.sqlite", table1, table2);

    storage.sync_schema();

    test1 t1;
    t1.val1 = "test";
    t1.val2 = "test";
    storage.insert(t1);

    test1 t1_copy;
    t1_copy.val1 = "test";
    t1_copy.val2 = "test";
    storage.insert(t1_copy);

    test2 t2;
    t2.fk_id = 1;
    t2.val1 = "test";
    t2.val2 = "test";
    storage.insert(t2);

    t2.fk_id = 2;

    storage.update(t2);
}
