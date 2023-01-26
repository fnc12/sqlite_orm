#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("issue937") {
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

    struct EmpBonus {
        int m_id;
        int m_empno;
        std::string m_received;  // date
        int m_type;
    };

    using namespace sqlite_orm;

    ::remove("SQLCookbook.sqlite");
    auto storage = make_storage("SQLCookbook.sqlite",
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
                                           make_column("loc", &Department::m_loc)),
                                make_table("Emp_bonus",
                                           make_column("id", &EmpBonus::m_id, primary_key().autoincrement()),
                                           make_column("empno", &EmpBonus::m_empno),
                                           make_column("received", &EmpBonus::m_received),
                                           make_column("type", &EmpBonus::m_type),
                                           foreign_key(&EmpBonus::m_empno).references(&Employee::m_empno)));
    storage.sync_schema();

    struct NamesAlias : alias_tag {
        static const std::string& get() {
            static const std::string res = "ENAME_AND_DNAME";
            return res;
        }
    };

    auto statement = storage.prepare(select(union_all(
        select(columns(as<NamesAlias>(&Department::m_deptname), as_optional(&Department::m_deptno))),
        select(union_all(select(columns(quote("--------------------"), std::optional<int>())),
                         select(columns(as<NamesAlias>(&Employee::m_ename), as_optional(&Employee::m_depno))))))));
    auto sql = statement.expanded_sql();
    auto rows = storage.execute(statement);
    {  //  issue953
        auto expression = select(
            columns(&Employee::m_empno, &Employee::m_ename, &Employee::m_job, &Employee::m_salary, &Employee::m_depno),
            where(c(std::make_tuple(&Employee::m_ename, &Employee::m_job, &Employee::m_salary))
                      .in(select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary),
                                 where(c(&Employee::m_job) == "Clerk")))));
        REQUIRE_NOTHROW(storage.prepare(expression));
    }
    {  //  issue969
        auto expression = select(
            columns(&Employee::m_empno, &Employee::m_ename, &Employee::m_job, &Employee::m_salary, &Employee::m_depno),
            where(in(std::make_tuple(&Employee::m_ename, &Employee::m_job, &Employee::m_salary),
                     intersect(select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary)),
                               select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary),
                                      where(c(&Employee::m_job) == "Clerk"))))));
        REQUIRE_NOTHROW(storage.prepare(expression));
    }
}
#endif
