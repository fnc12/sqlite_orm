#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("select asterisk") {
    using Catch::Matchers::UnorderedEquals;

    struct Employee {
        int id;
        std::string name;
        int age;
        std::string address;  //  optional
        double salary;  //  optional

        bool operator==(const Employee& other) const {
            return this->id == other.id && this->name == other.name && this->age == other.age &&
                   this->address == other.address && this->salary == other.salary;
        }
    };

    auto storage = make_storage({},
                                make_table("COMPANY",
                                           make_column("ID", &Employee::id, primary_key()),
                                           make_column("NAME", &Employee::name),
                                           make_column("AGE", &Employee::age),
                                           make_column("ADDRESS", &Employee::address),
                                           make_column("SALARY", &Employee::salary)));
    storage.sync_schema();

    //  create employees..
    Employee paul{-1, "Paul", 32, "California", 20000.0};
    Employee allen{-1, "Allen", 25, "Texas", 15000.0};
    Employee teddy{-1, "Teddy", 23, "Norway", 20000.0};
    Employee mark{-1, "Mark", 25, "Rich-Mond", 65000.0};
    Employee david{-1, "David", 27, "Texas", 85000.0};
    Employee kim{-1, "Kim", 22, "South-Hall", 45000.0};
    Employee james{-1, "James", 24, "Houston", 10000.0};

    //  insert employees. `insert` function returns id of inserted object..
    paul.id = storage.insert(paul);
    allen.id = storage.insert(allen);
    teddy.id = storage.insert(teddy);
    mark.id = storage.insert(mark);
    david.id = storage.insert(david);
    kim.id = storage.insert(kim);
    james.id = storage.insert(james);

    SECTION("asterisk") {
        auto allEmployeesTuples = storage.select(asterisk<Employee>());

        std::vector<std::tuple<int, std::string, int, std::string, double>> expected;

        expected.push_back(std::make_tuple(paul.id, "Paul", 32, "California", 20000.0));
        expected.push_back(std::make_tuple(allen.id, "Allen", 25, "Texas", 15000.0));
        expected.push_back(std::make_tuple(teddy.id, "Teddy", 23, "Norway", 20000.0));
        expected.push_back(std::make_tuple(mark.id, "Mark", 25, "Rich-Mond", 65000.0));
        expected.push_back(std::make_tuple(david.id, "David", 27, "Texas", 85000.0));
        expected.push_back(std::make_tuple(kim.id, "Kim", 22, "South-Hall", 45000.0));
        expected.push_back(std::make_tuple(james.id, "James", 24, "Houston", 10000.0));
        REQUIRE_THAT(allEmployeesTuples, UnorderedEquals(expected));
    }
    SECTION("object") {
        auto allEmployees = storage.select(object<Employee>());
        std::vector<Employee> expected;
        expected.push_back(paul);
        expected.push_back(allen);
        expected.push_back(teddy);
        expected.push_back(mark);
        expected.push_back(david);
        expected.push_back(kim);
        expected.push_back(james);
        REQUIRE_THAT(allEmployees, UnorderedEquals(expected));
    }
}
