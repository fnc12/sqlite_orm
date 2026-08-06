/**
 *  This example demonstrates how to use tables and SQL views with sqlite_orm and C++ reflection.
 *
 *  Views in SQL are virtual tables based on the result-set of a SELECT statement.
 *
 *  The reflection-based `make_table()` and `make_view()` factory functions don't require
 *  explicit column definitions - the columns are automatically derived from the type's
 *  non-static data members through reflection.
 *  Many table column constraints can be specified through C++ annotations on the data members,
 *  table-level constraints will continue to be passed to the `make_table()` function.
 */

#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <string>
#include <cstdio>  // std::remove

#ifdef SQLITE_ORM_WITH_VIEW
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
using namespace sqlite_orm;
using std::cout;
using std::endl;
using std::string;

void handle_contract_violation(std::contracts::contract_violation const& violation) noexcept {}

// Base tables
struct[[= "employee"_orm_name]] Employee {
    [[= primary_key()]] int64 id;
    [[= collate_nocase()]] std::string name;
    [[= not_null()]] int64 department_id;
    [[= default_value(0.)]] double salary;
};

struct[[= "department"_orm_name]] Department {
    [[= primary_key()]] int64 id;
    [[= collate_nocase()]] std::string name;
    [[= collate_nocase()]] std::string location;
};

// View objects - note how we only define the struct fields, no column mappings needed!
// The fields are automatically mapped through C++ reflection

// View 1: High earners (employees earning more than 60000)
struct[[= "high_earner"_orm_name]] HighEarner {
    int64 id;
    std::string name;
    double salary;
};

// View 2: Department summary with employee count and average salary
struct[[= "department_summary"_orm_name]] DepartmentSummary {
    std::string department_name;
    int employee_count;
    double avg_salary;
};

// View 3: Complete employee information with department details (join result)
struct[[= "employee_details"_orm_name]] EmployeeDetail {
    int64 id;
    std::string employee_name;
    double salary;
    std::string department_name;
    std::string location;
};

inline auto initStorage(const std::string& path) {
    return make_storage(
        path,
        // Define base tables
        make_table<Department>(),
        make_table<Employee>(foreign_key(&Employee::department_id).references(&Department::id)),

        // Define views - notice how we only specify the SELECT statement.
        // The column mappings and view name are derived from the view object type
        // (column names from non-static data members; view name from the optional
        // `[[="…"_orm_name]]` annotation, falling back to the type's identifier).

        // View 1: Filter high earners
        make_view<HighEarner>(
            select(columns(&Employee::id, &Employee::name, &Employee::salary), where(c(&Employee::salary) > 60000.0))),

        // View 2: Aggregate data by department
        make_view<DepartmentSummary>(select(columns(&Department::name, count(&Employee::id), avg(&Employee::salary)),
                                            left_join<Employee>(on(c(&Employee::department_id) == &Department::id)),
                                            group_by(&Department::name))),

        // View 3: Join employees with departments
        make_view<EmployeeDetail>(
            select(columns(&Employee::id, &Employee::name, &Employee::salary, &Department::name, &Department::location),
                   join<Department>(on(c(&Employee::department_id) == &Department::id)))));
}
#endif

int main() {
#ifdef ENABLE_THIS_EXAMPLE
    try {
        cout << "=== SQL Views Example ===" << endl << endl;

        std::remove("view_example.sqlite");
        auto storage = initStorage("view_example.sqlite");
        storage.sync_schema();

        storage.transaction([&storage]() {
            // Insert departments
            cout << "Inserting departments..." << endl;
            auto engineeringId = storage.insert(Department{0, "Engineering", "San Francisco"});
            auto salesId = storage.insert(Department{0, "Sales", "New York"});
            auto hrId = storage.insert(Department{0, "HR", "Chicago"});
            auto marketingId = storage.insert(Department{0, "Marketing", "Austin"});

            // Insert employees
            cout << "Inserting employees..." << endl;
            storage.insert(Employee{0, "Alice Johnson", engineeringId, 95000.0});
            storage.insert(Employee{0, "Bob Smith", engineeringId, 87000.0});
            storage.insert(Employee{0, "Carol Williams", salesId, 72000.0});
            storage.insert(Employee{0, "David Brown", salesId, 68000.0});
            storage.insert(Employee{0, "Eve Davis", hrId, 55000.0});
            storage.insert(Employee{0, "Frank Miller", marketingId, 62000.0});
            storage.insert(Employee{0, "Grace Wilson", engineeringId, 78000.0});
            storage.insert(Employee{0, "Henry Moore", salesId, 45000.0});

            return true;
        });

        cout << endl;

        // Query View 1: High Earners
        cout << "=== View 1: High Earners (salary > $60,000) ===" << endl;
        auto highEarners = storage.select(object<HighEarner>());
        cout << "ID\tName\t\t\tSalary" << endl;
        cout << "---\t----\t\t\t------" << endl;
        for (const auto& earner: highEarners) {
            cout << earner.id << '\t' << earner.name << (earner.name.length() < 16 ? "\t" : "") << '\t' << "$"
                 << earner.salary << endl;
        }
        cout << "Total high earners: " << highEarners.size() << endl << endl;

        // Query View 2: Department Summary
        cout << "=== View 2: Department Summary ===" << endl;
        auto summaries = storage.select(object<DepartmentSummary>());
        cout << "Department\tEmployees\tAvg Salary" << endl;
        cout << "----------\t---------\t----------" << endl;
        for (const auto& summary: summaries) {
            cout << summary.department_name << '\t';
            if (summary.department_name.length() < 8)
                cout << '\t';
            cout << summary.employee_count << "\t\t";
            if (summary.employee_count > 0) {
                cout << "$" << static_cast<int>(summary.avg_salary);
            } else {
                cout << "N/A";
            }
            cout << endl;
        }
        cout << endl;

        // Query View 3: Employee Details
        cout << "=== View 3: Employee Details (with Department Info) ===" << endl;
        auto details = storage.select(object<EmployeeDetail>());
        cout << "ID\tEmployee\t\tSalary\t\tDepartment\tLocation" << endl;
        cout << "--\t--------\t\t------\t\t----------\t--------" << endl;
        for (const auto& detail: details) {
            cout << detail.id << '\t' << detail.employee_name << (detail.employee_name.length() < 16 ? "\t" : "")
                 << '\t' << "$" << detail.salary << '\t' << detail.department_name << '\t';
            if (detail.department_name.length() < 8)
                cout << '\t';
            cout << detail.location << endl;
        }
        cout << endl;

        // Demonstrate that you can also query views with conditions
        cout << "=== View Query with Additional Filter ===" << endl;
        cout << "High earners from Engineering department:" << endl;
        auto engineeringHighEarners = storage.select(
            object<EmployeeDetail>(),
            where(c(&EmployeeDetail::department_name) == "Engineering" and c(&EmployeeDetail::salary) > 60000.0));

        for (const auto& emp: engineeringHighEarners) {
            cout << "  - " << emp.employee_name << ": $" << emp.salary << endl;
        }
        cout << endl;

        // Show view metadata
        cout << "=== View Metadata ===" << endl;
        auto viewNames = storage.view_names();
        cout << "Views in database:" << endl;
        for (const auto& viewName: viewNames) {
            cout << "  - " << viewName << endl;
        }
        cout << endl;

        cout << "Example completed successfully!" << endl;
    } catch (const std::system_error& e) {
        cout << "System error: " << e.what() << " (code " << e.code() << ")" << endl;
    }
#endif

    return 0;
}
