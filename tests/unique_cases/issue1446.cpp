#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3006019
using namespace sqlite_orm;

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED

TEST_CASE("issue1446") {
    struct Employee {
        int m_empno;
        std::string m_ename;
        std::string m_job;
        std::optional<int> m_mgr;
        std::string m_hiredate;
        double m_salary;
        std::optional<double> m_commission;
        int m_depno;
    };

    struct Department {
        int m_deptno;
        std::string m_deptname;
        std::string m_loc;
    };

    using namespace sqlite_orm;

    auto storage = make_storage("",
                                make_table("Emp",
                                           make_column("empno", &Employee::m_empno, primary_key().autoincrement()),
                                           make_column("ename", &Employee::m_ename),
                                           make_column("job", &Employee::m_job),
                                           make_column("mgr", &Employee::m_mgr),
                                           make_column("hiredate", &Employee::m_hiredate),
                                           make_column("salary", &Employee::m_salary),
                                           make_column("comm", &Employee::m_commission),
                                           make_column("depno", &Employee::m_depno),
                                           foreign_key(&Employee::m_depno).references(&Department::m_deptno)),
                                make_table("Dept",
                                           make_column("deptno", &Department::m_deptno, primary_key().autoincrement()),
                                           make_column("deptname", &Department::m_deptname),
                                           make_column("loc", &Department::m_loc, null())));
    storage.sync_schema(true);
    storage.insert(Department{10, "Accounts", "New York"});
    storage.insert(Employee{1, "Paul", "Salesman", 2, "2002-02-12", 20000.0, 0.0, 1});
    storage.insert(Employee{2, "Allen", "Salesman", 2, "2002-02-12", 20000.0, 0.0, 1});
    for (auto&& [obj, obj2, col]:
         storage.iterate(select(columns(object<Employee>(true), object<Department>(true), &Department::m_deptno),
                                from<Employee>(),
                                join<Department>(on(c(&Employee::m_depno) == c(&Department::m_deptno)))))) {
        STATIC_REQUIRE(std::is_same_v<decltype(obj), Employee>);
        STATIC_REQUIRE(std::is_same_v<decltype(obj2), Department>);
        STATIC_REQUIRE(std::is_same_v<decltype(col), int>);
    }
}
#endif
#endif
#endif