#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <string>

using std::cout;
using std::endl;

struct Employee {
    int id = 0;
    std::string name;
    std::string department;
    double salary = 0;
};

int main(int, char** argv) {
    cout << "path = " << argv[0] << endl;

    using namespace sqlite_orm;
    auto storage = make_storage("",
                                make_table("employees",
                                           make_column("id", &Employee::id, primary_key().autoincrement()),
                                           make_column("name", &Employee::name),
                                           make_column("department", &Employee::department),
                                           make_column("salary", &Employee::salary)));
    storage.sync_schema();

    storage.transaction([&storage] {
        storage.insert(Employee{-1, "Alice", "Engineering", 90000});
        storage.insert(Employee{-1, "Bob", "Engineering", 85000});
        storage.insert(Employee{-1, "Charlie", "Engineering", 85000});
        storage.insert(Employee{-1, "Diana", "Sales", 70000});
        storage.insert(Employee{-1, "Eve", "Sales", 65000});
        storage.insert(Employee{-1, "Frank", "Sales", 65000});
        storage.insert(Employee{-1, "Grace", "HR", 75000});
        storage.insert(Employee{-1, "Hank", "HR", 72000});
        return true;
    });

    cout << endl;

    //  SELECT name, department, salary, ROW_NUMBER() OVER (PARTITION BY department ORDER BY salary DESC)
    //  FROM employees
    {
        cout << "=== ROW_NUMBER with PARTITION BY ===" << endl;
        auto result = storage.select(
            columns(&Employee::name,
                    &Employee::department,
                    &Employee::salary,
                    row_number().over(partition_by(&Employee::department), order_by(&Employee::salary).desc())));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\t" << get<2>(row) << "\t#" << get<3>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT name, salary, RANK() OVER (ORDER BY salary DESC)
    //  FROM employees
    {
        cout << "=== RANK ===" << endl;
        auto result = storage.select(
            columns(&Employee::name, &Employee::salary, rank().over(order_by(&Employee::salary).desc())));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\trank=" << get<2>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT name, salary, DENSE_RANK() OVER (ORDER BY salary DESC)
    //  FROM employees
    {
        cout << "=== DENSE_RANK ===" << endl;
        auto result = storage.select(
            columns(&Employee::name, &Employee::salary, dense_rank().over(order_by(&Employee::salary).desc())));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\tdense_rank=" << get<2>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT name, salary, NTILE(3) OVER (ORDER BY salary DESC)
    //  FROM employees
    {
        cout << "=== NTILE(3) ===" << endl;
        auto result = storage.select(
            columns(&Employee::name, &Employee::salary, ntile(3).over(order_by(&Employee::salary).desc())));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\tbucket=" << get<2>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT name, salary,
    //    LAG(name, 1) OVER (ORDER BY salary DESC),
    //    LEAD(name, 1) OVER (ORDER BY salary DESC)
    //  FROM employees
    {
        cout << "=== LAG / LEAD ===" << endl;
        auto w = order_by(&Employee::salary).desc();
        auto result = storage.select(columns(&Employee::name,
                                             &Employee::salary,
                                             lag(&Employee::name, 1).over(order_by(&Employee::salary).desc()),
                                             lead(&Employee::name, 1).over(order_by(&Employee::salary).desc())));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\tprev=" << get<2>(row) << "\tnext=" << get<3>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT name, department, salary,
    //    FIRST_VALUE(name) OVER (PARTITION BY department ORDER BY salary DESC),
    //    LAST_VALUE(name) OVER (PARTITION BY department ORDER BY salary DESC
    //      RANGE BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING)
    //  FROM employees
    {
        cout << "=== FIRST_VALUE / LAST_VALUE ===" << endl;
        auto result = storage.select(columns(
            &Employee::name,
            &Employee::department,
            &Employee::salary,
            first_value(&Employee::name).over(partition_by(&Employee::department), order_by(&Employee::salary).desc()),
            last_value(&Employee::name)
                .over(partition_by(&Employee::department),
                      order_by(&Employee::salary).desc(),
                      range(unbounded_preceding(), unbounded_following()))));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\t" << get<2>(row) << "\tfirst=" << get<3>(row)
                 << "\tlast=" << get<4>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT name, department, salary,
    //    SUM(salary) OVER (PARTITION BY department ORDER BY salary
    //      ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
    //  FROM employees
    {
        cout << "=== Running SUM with frame spec ===" << endl;
        auto result = storage.select(columns(&Employee::name,
                                             &Employee::department,
                                             &Employee::salary,
                                             sum(&Employee::salary)
                                                 .over(partition_by(&Employee::department),
                                                       order_by(&Employee::salary),
                                                       rows(unbounded_preceding(), current_row()))));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\t" << get<2>(row) << "\trunning_sum=";
            if (auto& v = get<3>(row)) {
                cout << *v;
            }
            cout << endl;
        }
        cout << endl;
    }

    //  SELECT name, salary,
    //    PERCENT_RANK() OVER (ORDER BY salary),
    //    CUME_DIST() OVER (ORDER BY salary)
    //  FROM employees
    {
        cout << "=== PERCENT_RANK / CUME_DIST ===" << endl;
        auto result = storage.select(columns(&Employee::name,
                                             &Employee::salary,
                                             percent_rank().over(order_by(&Employee::salary)),
                                             cume_dist().over(order_by(&Employee::salary))));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\tpct_rank=" << get<2>(row) << "\tcume_dist=" << get<3>(row)
                 << endl;
        }
        cout << endl;
    }

    //  Named window:
    //  SELECT name, salary,
    //    ROW_NUMBER() OVER win,
    //    SUM(salary) OVER win
    //  FROM employees
    //  WINDOW win AS (ORDER BY salary DESC)
    {
        cout << "=== Named window ===" << endl;
        auto result = storage.select(columns(&Employee::name,
                                             &Employee::salary,
                                             row_number().over(window_ref("win")),
                                             sum(&Employee::salary).over(window_ref("win"))),
                                     window("win", order_by(&Employee::salary).desc()));
        for (auto& row: result) {
            cout << get<0>(row) << "\t" << get<1>(row) << "\trow#=" << get<2>(row) << "\tsum=";
            if (auto& v = get<3>(row)) {
                cout << *v;
            }
            cout << endl;
        }
        cout << endl;
    }

    return 0;
}
