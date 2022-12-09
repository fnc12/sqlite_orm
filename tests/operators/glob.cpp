/**
 *  Obtained from here https://www.tutlane.com/tutorial/sqlite/sqlite-glob-operator
 */
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <vector>  //  std::vector
#include <algorithm>  //  std::find_if, std::count

using namespace sqlite_orm;

namespace {
    struct Employee {
        int id = 0;
        std::string firstName;
        std::string lastName;
        float salary = 0;
        int deptId = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Employee() = default;
        Employee(int id, std::string firstName, std::string lastName, float salary, int deptId) :
            id{id}, firstName{move(firstName)}, lastName{move(lastName)}, salary{salary}, deptId{deptId} {}
#endif
    };
}
TEST_CASE("Glob") {
    auto storage = make_storage({},
                                make_table("emp_master",
                                           make_column("emp_id", &Employee::id, primary_key().autoincrement()),
                                           make_column("first_name", &Employee::firstName),
                                           make_column("last_name", &Employee::lastName),
                                           make_column("salary", &Employee::salary),
                                           make_column("dept_id", &Employee::deptId)));
    storage.sync_schema();
    {
        std::vector<Employee> employees = {
            {1, "Honey", "Patel", 10100, 1},
            {2, "Shweta", "Jariwala", 19300, 2},
            {3, "Vinay", "Jariwala", 35100, 3},
            {4, "Jagruti", "Viras", 9500, 2},
            {5, "Shweta", "Rana", 12000, 3},
            {6, "sonal", "Menpara", 13000, 1},
            {7, "Yamini", "Patel", 10000, 2},
            {8, "Khyati", "Shah", 500000, 3},
            {9, "Shwets", "Jariwala", 19400, 2},
        };
        storage.replace_range(employees.begin(), employees.end());
    }

    auto expectIds = [](const std::vector<Employee>& employees, const std::vector<decltype(Employee::id)> ids) {
        for(auto expectedId: ids) {
            REQUIRE(find_if(employees.begin(), employees.end(), [expectedId](auto& employee) {
                        return employee.id == expectedId;
                    }) != employees.end());
        }
        return false;
    };
    {
        auto employees = storage.get_all<Employee>(where(glob(&Employee::salary, "1*")));
        REQUIRE(employees.size() == 6);
        expectIds(employees, {1, 2, 5, 6, 7, 9});
    }
    {
        auto employees = storage.get_all<Employee>(where(glob(&Employee::firstName, "Shwet?")));
        REQUIRE(employees.size() == 3);
        expectIds(employees, {2, 5, 9});
    }
    {
        auto employees = storage.get_all<Employee>(where(glob(&Employee::lastName, "[A-J]*")));
        REQUIRE(employees.size() == 3);
        expectIds(employees, {2, 3, 9});
    }
    {
        auto employees = storage.get_all<Employee>(where(glob(&Employee::lastName, "[^A-J]*")));
        REQUIRE(employees.size() == 6);
        expectIds(employees, {1, 4, 5, 6, 7, 8});
    }
    {
        auto rows = storage.select(glob(&Employee::firstName, "S*"));
        REQUIRE(rows.size() == 9);
        auto trueValuesCount = std::count(rows.begin(), rows.end(), true);
        REQUIRE(trueValuesCount == 3);
    }
    {
        auto rows = storage.select(glob(distinct(&Employee::firstName), "S*"));
        REQUIRE(rows.size() == 2);
        auto trueValuesCount = std::count(rows.begin(), rows.end(), true);
        REQUIRE(trueValuesCount == 1);
    }
    {
        auto rows = storage.select(columns(not glob(&Employee::firstName, "S*")));
        REQUIRE(rows.size() == 9);
        auto trueValuesCount = std::count(rows.begin(), rows.end(), std::tuple<bool>{true});
        REQUIRE(trueValuesCount == 6);
    }
}
