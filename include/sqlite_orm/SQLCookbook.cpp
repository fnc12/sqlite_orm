#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <vector>
#include <iostream>
#include <cassert>

// #include "SQLCookbook.h"
#include <sqlite_orm/SQLCookbook.h>
#include <sqlite_orm/ForeignKeyHolder.h>

void SQL1_8();
void SQL1_12();
void SQL1_13();
void SQL2_3();
void SQL2_5();
void SQL2_6();
void SQL3_1();
void SQL3_2();
void SQL3_3();
void SQL3_4();
void SQL3_5();
void SQL3_6();
void SQL3_9();
void Except();
void usingUpdate();
void usingDelete();
void usingObjectAPI();
void testMigrationAgent();
void loadv0();

int main(int argc, char* argv[])
{
	using namespace sqlite_orm;

	if (argc > 1)
	{
		std::string cmd = argv[1];
		if (cmd == "-migrate")
		{
			testMigrationAgent();
			return 0;
		}
		if(cmd == "-load") {
			loadv0();
			return 0;
		}
	}

	try
	{
		// SchemaManager sm(storage, temp_storage);

        
		// storage.backup_to("Temp.sqlite");

		// sm.load_drop_sync_replace<Employee>();
		// sm.load_drop_sync_replace<Employee>();



		// sm.guarded_sync_schema();
		if(storage.pragma.user_version() == 1) {
			// auto st = create_storage_ver_1();

		}
		storage.sync_schema(true);

		// auto pair = sm.find_duplicate_in_column<Employee, &Employee::m_ename>();
		// if (pair.first)
		// {
		// 	// duplicates found
		// 	auto x = *pair.second;
		// 	std::ignore = x;
		// }


		// storage.remove_all<Album>();
		// storage.remove_all<Artist>();
		// storage.remove_all<EmpBonus>();
		// storage.remove_all<Employee>();
		// storage.remove_all<Department>();
	}
	catch(std::exception& ex)
	{
		auto s = ex.what();
		std::ignore = s;
	}
#if 0
	std::vector<Artist> art =
	{
		Artist{1, "Elton John"},
		Artist{2, "Prince"}
	};

	std::vector<Album> albums =
	{
		Album{1, 1}
	};

	
	std::vector<Employee> vec
	{
		Employee{ 7369, "Smith", "Clerk", 7902, "17-DEC-1980",800,std::nullopt, 20 },
			Employee{ 7499, "Allen", "SalesMan", 7698, "20-FEB-1981", 1600, 300, 30 },
			Employee{ 7521,"Ward", "SalesMan", 7698,"22-feb-1981",1250,500, 30 },
			Employee{ 7566,"Jones", "Manager", 7839, "02-abr-1981",2975, std::nullopt,20 },
			Employee{ 7654,"Martin","SalesMan", 7698, "28-sep-1981", 1250,1400,30 },
			Employee{ 7698,"Blake", "Manager", 7839, "01-may-1981", 2850, std::nullopt, 30 },
			Employee{ 7782, "Clark", "Manager", 7839, "09-jun-1981", 2450, std::nullopt, 10 },
			Employee{ 7788, "Scott", "Analyst", 7566, "09-Dec-1982", 3000, std::nullopt, 20 },
			Employee{ 7839, "King", "President", std::nullopt, "17-nov-1981", 5000, std::nullopt,10 },
			Employee{ 7844,"Turner","SalesMan", 7698, "08-Sep-1981", 1500, 0, 30 },
			Employee{ 7876, "Adams", "Clerk", 7788, "12-JAN-1983", 1100, std::nullopt, 20 },
			Employee{ 7900,"James", "Clerk", 7698,"03-DEC-1981", 950, std::nullopt, 30 },
			Employee{ 7902,"Ford", "Analyst", 7566, "03-DEC-1981", 3000, std::nullopt, 20 },
			Employee{ 7934, "Miller", "Clerk", 7782,"23-JAN-1982", 1300, std::nullopt, 10 }
	};

	std::vector<Department> des =
	{
		Department{10, "Accounting", "New York"},
		Department{20, "Research", "Dallas"},
		Department{30, "Sales", "Chicago"},
		Department{40, "Operations", "Boston"}
	};

	std::vector<EmpBonus> bonuses =
	{
		// EmpBonus{-1, 7369, "14-Mar-2005", 1},
		// EmpBonus{-1, 7900, "14-Mar-2005", 2},
		// EmpBonus{-1, 7788, "14-Mar-2005", 3},
		EmpBonus{-1, 7934, "17-Mar-2005", 1},
		EmpBonus{-1, 7934, "15-Feb-2005", 2},
		EmpBonus{-1, 7839, "15-Feb-2005", 3},
		EmpBonus{-1, 7782, "15-Feb-2005", 1}
	};

	try
	{
		storage.replace_range(art.begin(), art.end());
		storage.replace_range(albums.begin(), albums.end());
		storage.replace_range(des.begin(), des.end());
		storage.replace_range(vec.begin(), vec.end());
		storage.insert_range(bonuses.begin(), bonuses.end());
	}
	catch(std::exception& ex)
	{
		auto s = ex.what();
		std::ignore = s;
	}
#endif
	usingObjectAPI();
	usingDelete();
	usingUpdate();
	SQL1_8();
	SQL1_12();
	SQL1_13();
	SQL2_3();
	SQL2_5();
	SQL2_6();
	SQL3_1();
	SQL3_2();
	SQL3_3();
	SQL3_4();
	SQL3_5();
	SQL3_6();
	SQL3_9();
	Except();
}


void loadv0() {
	foreign_key_holder fk(v0::storage);

	try {
            storage.remove_all<Album>();
            storage.remove_all<Artist>();
            storage.remove_all<EmpBonus>();
            storage.remove_all<Employee>();
            storage.remove_all<Department>();
        } catch(std::exception& ex) {
            auto s = ex.what();
            std::ignore = s;
        }
		storage.sync_schema(true);

        std::vector<Artist> art = {Artist{1, "Elton John"}, Artist{2, "Prince"}};

        std::vector<Album> albums = {Album{1, 1}};

        std::vector<Employee> vec{
            Employee{7369, "Smith", "Clerk", 7902, "17-DEC-1980", 800, std::nullopt, 20},
            Employee{7499, "Allen", "SalesMan", 7698, "20-FEB-1981", 1600, 300, 30},
            Employee{7521, "Ward", "SalesMan", 7698, "22-feb-1981", 1250, 500, 30},
            Employee{7566, "Jones", "Manager", 7839, "02-abr-1981", 2975, std::nullopt, 20},
            Employee{7654, "Martin", "SalesMan", 7698, "28-sep-1981", 1250, 400, 30},
            Employee{7698, "Blake", "Manager", 7839, "01-may-1981", 2850, std::nullopt, 30},
            Employee{7782, "Clark", "Manager", 7839, "09-jun-1981", 2450, std::nullopt, 10},
            Employee{7788, "Scott", "Analyst", 7566, "09-Dec-1982", 3000, std::nullopt, 20},
            Employee{7839, "King", "President", std::nullopt, "17-nov-1981", 5000, std::nullopt, 10},
            Employee{7844, "Turner", "SalesMan", 7698, "08-Sep-1981", 1500, 0, 30},
            Employee{7876, "Adams", "Clerk", 7788, "12-JAN-1983", 1100, std::nullopt, 20},
            Employee{7900, "James", "Clerk", 7698, "03-DEC-1981", 950, std::nullopt, 30},
            Employee{7902, "Ford", "Analyst", 7566, "03-DEC-1981", 3000, std::nullopt, 20},
            Employee{7934, "Miller", "Clerk", 7782, "23-JAN-1982", 1300, std::nullopt, 10}};

        std::vector<Department> des = {Department{10, "Accounting", "New York"},
                                       Department{20, "Research", "Dallas"},
                                       Department{30, "Sales", "Chicago"},
                                       Department{40, "Operations", "Boston"}};

        std::vector<EmpBonus> bonuses = {// EmpBonus{-1, 7369, "14-Mar-2005", 1},
                                         // EmpBonus{-1, 7900, "14-Mar-2005", 2},
                                         // EmpBonus{-1, 7788, "14-Mar-2005", 3},
                                         EmpBonus{-1, 7934, "17-Mar-2005", 1},
                                         EmpBonus{-1, 7934, "15-Feb-2005", 2},
                                         EmpBonus{-1, 7839, "15-Feb-2005", 3},
                                         EmpBonus{-1, 7782, "15-Feb-2005", 1}};

        try {
            storage.replace_range(art.begin(), art.end());
            storage.replace_range(albums.begin(), albums.end());
            storage.replace_range(des.begin(), des.end());
            storage.replace_range(vec.begin(), vec.end());
            storage.insert_range(bonuses.begin(), bonuses.end());
        } catch(std::exception& ex) {
            auto s = ex.what();
            std::ignore = s;
        }

}

void CreateView()
{
	struct Salary_Qualified	// this would be the interface of the view
	{
		std::string m_ename;
		std::string m_job;
	};
	auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_job), where(in(&Employee::m_deptno, { 10,20 }))));

	// make_view("salary_qualified", &Salary_Qualified::m_ename, &Salary_Qualified::m_job, as(statement));
}

void SQL1_6()
{
	struct SalaryAlias : alias_tag {
		static const std::string& get() {
			static const std::string res = "SALARY";
			return res;
		}
	};


	//    select * 
//    from (select salary, comm as commission 
//          from emp)
//    where salary< 5000
	// storage.prepare(select(asterisk<Employee>(), from<Employee>()))
	struct EmpCte {
		int salary = 0;
		int comm = 0;
	};
	auto empcte = make_table("emp_inter",
		make_column("salary", &EmpCte::salary),
		make_column("comm", &EmpCte::comm));
	// auto statement = storage.prepare(select(asterisk<EmpCte>(), from<EmpCte>(), where(c(&EmpCte::salary) < 5000)));
	// auto sql = statement.expanded_sql();
	// auto rows = storage.execute(statement);

}

void SQL1_8()
{
	auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_salary,
		case_<std::string>()
		.when(lesser_or_equal(&Employee::m_salary, 2000), then("UNDERPAID"))
		.when(greater_or_equal(&Employee::m_salary, 4000), then("OVERPAID"))
		.else_("OK").end())));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);
}
void SQL1_12()
{
	// Transforming null values into real values
	// SELECT COALESCE(comm,0), comm FROM EMP

	auto statement = storage.prepare(select(columns(coalesce<double>(&Employee::m_commission, 0), &Employee::m_commission)));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);
}

void SQL1_13()
{
	auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_job), where(in(&Employee::m_deptno, { 10,20 }))));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);

}

void SQL2_3()
{
	// SELECT ename, job from EMP order by substring(job, len(job)-1,2)
	auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_job), order_by(substr(&Employee::m_job, length(&Employee::m_job) - 1, 2))));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);

}

void SQL2_5()
{
	auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_salary, &Employee::m_commission), 
		order_by(case_<int>().when(is_null(&Employee::m_commission), then(0)).else_(1).end()).desc()));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);

	{
		auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_salary, &Employee::m_commission),
			order_by(is_null(&Employee::m_commission)).asc()));
		auto sql = statement.expanded_sql();
		auto rows = storage.execute(statement);

	}
	{
		auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_salary, &Employee::m_commission),
			order_by(2))); // DOES NOT WORK!
		auto sql = statement.expanded_sql();
		auto rows = storage.execute(statement);


	}
}

void SQL2_6()
{
	// Sorting on a data dependent key
	auto statement = storage.prepare(select(columns(&Employee::m_ename, &Employee::m_salary, &Employee::m_commission, &Employee::m_job),
		order_by(case_<double>().when(is_equal(&Employee::m_job, "SalesMan"), then(&Employee::m_commission)).else_(&Employee::m_salary).end()).desc()));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);

	{
		auto expression = select(columns(&Employee::m_ename, &Employee::m_salary, &Employee::m_commission, &Employee::m_job),
			order_by(case_<double>().when(is_equal(&Employee::m_job, "SalesMan"), then(&Employee::m_commission)).else_(&Employee::m_salary).end()).desc());
		std::string sql = storage.dump(expression);
		auto statement = storage.prepare(expression);
		auto rows = storage.execute(statement);
	}
	{
		try
		{
			auto expression = insert(into<Employee>(), columns(&Employee::m_ename, &Employee::m_salary, &Employee::m_commission, &Employee::m_job), values(std::make_tuple("Juan", 224000, 200, "Eng")));
			std::string sql = storage.dump(expression);
			auto statement = storage.prepare(expression);
			storage.execute(statement);
		}
		catch(std::exception& ex)
		{
			std::string s = ex.what();
			std::ignore = s;
		}
	}
}


void SQL3_1()
{
	struct NamesAlias : alias_tag {
		static const std::string& get() {
			static const std::string res = "ENAME_AND_DNAME";
			return res;
		}
	};

	try
	{
		/*
		 *
// SELECT "Dept"."deptname" AS ENAME_AND_DNAME, "Dept"."deptno" FROM 'Dept' UNION ALL SELECT (QUOTE("------------------")), NULL UNION ALL
// SELECT "Emp"."ename" AS ENAME_AND_DNAME, "Emp"."deptno" FROM 'Emp'

		 *
		 *
		 */
		auto statement = storage.prepare(
			select(union_all(
				select(columns(as<NamesAlias>(&Department::m_deptname), as_optional(&Department::m_deptno))),
				select(union_all(
					select(columns(quote("--------------------"), std::optional<int>())),
					select(columns(as<NamesAlias>(&Employee::m_ename), as_optional(&Employee::m_deptno))))))));
			
		auto sql = statement.expanded_sql();
		auto rows = storage.execute(statement);
	}
	catch(std::exception& ex)
	{
		auto s = ex.what();
		std::ignore = s;
	}
}

void SQL3_2()
{
	struct EmpDptNo : alias_tag
	{
		static constexpr std::string get() { return "EmpDeptNo"; }
	};
	struct DeptDptNo : alias_tag
	{
		static constexpr std::string get() { return "DeptDeptNo"; }
	};
	using als_e = alias_e<Employee>;
	using als_d = alias_d<Department>;

	auto statement = storage.prepare(select(columns(alias_column<als_e>(&Employee::m_ename), alias_column<als_d>(&Department::m_loc), as<EmpDptNo>(alias_column<als_e>(&Employee::m_deptno)),
		as<DeptDptNo >(alias_column<als_d>(&Department::m_deptno))), from<als_e>(), join<als_d>(
			on(c(alias_column<als_e>(&Employee::m_deptno)) == alias_column<als_d>(&Department::m_deptno))), where(alias_column<als_e>(&Employee::m_deptno) == c(10))));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);

}
void SQL3_3()
{
#if 1
	// select empno, ename, job, salary, depno from Emp
	// 	where(ename, job, salary) in
	// (select ename, job, salary from emp
	// 	intersect
	// 	select ename, job, salary from emp where job = "Clerk")

	// storage.prepare(select(columns(&Employee::m_empno, &Employee::m_ename, &Employee::m_job, &Employee::m_salary, &Employee::m_deptno)),
	// 	where(in(std::tuple(&Employee::m_ename, &Employee::m_job, &Employee::m_salary),
	// 		select(intersect(
	// 			select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary)),
	// 			select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary), where(c(&Employee::m_job) == "Clerk"))
	// 			)))));
	// THIS ONE DOES NOT RUN!
	try
	{
		auto statement = storage.prepare(select(columns(&Employee::m_empno, &Employee::m_ename, &Employee::m_job, &Employee::m_salary, &Employee::m_deptno),
			where(in(std::make_tuple(&Employee::m_ename, &Employee::m_job, &Employee::m_salary),
				intersect(
					select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary)),
					select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary), where(c(&Employee::m_job) == "Clerk")
					))))));
		auto sql = statement.expanded_sql();
		auto rows0 = storage.execute(statement);
	}
	catch(std::exception& ex)
	{
		auto s = ex.what();
		std::ignore = s;
	}
	// SELECT "Emp"."empno", "Emp"."ename", "Emp"."job", "Emp"."salary", "Emp"."deptno" FROM 'Emp' WHERE
	// (("Emp"."ename", "Emp"."job", "Emp"."salary") IN (
	// SELECT "Emp"."ename", "Emp"."job", "Emp"."salary" FROM 'Emp'
	// INTERSECT
	// SELECT "Emp"."ename", "Emp"."job", "Emp"."salary" FROM 'Emp' WHERE (("Emp"."job" = “Clerk”))))

	try
	{
		auto statement = storage.prepare(select(columns(&Employee::m_empno, &Employee::m_ename, &Employee::m_job, &Employee::m_salary, &Employee::m_deptno),
			where(c(std::make_tuple(&Employee::m_ename, &Employee::m_job, &Employee::m_salary)).in(
				intersect(
					select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary)),
					select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary), where(c(&Employee::m_job) == "Clerk")
					))))));
		auto sql = statement.expanded_sql();
		auto rows = storage.execute(statement);
	}
	catch(std::exception& ex)
	{
		auto s = ex.what();
		std::ignore = s;
	}

	{
		try
		{
			// SELECT "Emp"."empno", "Emp"."ename", "Emp"."job", "Emp"."salary", "Emp"."deptno" FROM 'Emp'
			// WHERE(("Emp"."ename", "Emp"."job", "Emp"."salary")
			// IN(SELECT "Emp"."ename", "Emp"."job", "Emp"."salary" FROM 'Emp' WHERE(("Emp"."job" = "Clerk" ))))

			auto statement = storage.prepare(select(columns(
				&Employee::m_empno, &Employee::m_ename, &Employee::m_job, &Employee::m_salary, &Employee::m_deptno),
				where(in(std::make_tuple(&Employee::m_ename, &Employee::m_job, &Employee::m_salary), select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary), where(c(&Employee::m_job) == "Clerk"))))));
			auto sql = statement.expanded_sql();
			auto rows = storage.execute(statement);
			{
				auto statement = storage.prepare(select(columns(
					&Employee::m_empno, &Employee::m_ename, &Employee::m_job, &Employee::m_salary, &Employee::m_deptno),
					where(c(std::make_tuple(&Employee::m_ename, &Employee::m_job, &Employee::m_salary)).in(select(columns(&Employee::m_ename, &Employee::m_job, &Employee::m_salary), where(c(&Employee::m_job) == "Clerk"))))));

			}
		}
		catch(std::exception& ex)
		{
			auto s = ex.what();
			std::ignore = s;
		}

	}
#endif
}

void Except()
{
	// Find all artists ids of artists who do not have any album in the albums table:
	// SELECT ArtistId FROM artists EXCEPT SELECT ArtistId FROM albums;

	auto statement = storage.prepare(select(except(select(&Artist::m_id), select(&Album::m_artist_id))));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);
}

void SQL3_4()
{
	auto statement = storage.prepare(select(except(select(&Department::m_deptno), select(&Employee::m_deptno))));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);

	{
		using als_d = alias_d<Department>;

		auto statement = storage.prepare(select(alias_column<als_d>(&Department::m_deptno), from<als_d>(), where(
			not exists(select(1, from<Employee>(), where(c(alias_column<als_d>(&Department::m_deptno)) == &Employee::m_deptno))))));
		auto sql = statement.expanded_sql();
		auto rows = storage.execute(statement);

	}
}

void SQL3_5()
{
	/*
	* Retrieving rows from one table that do not correspond to rows in another...
	* 
	 *	select d.* from dept d left outer join emp e
		on(d.deptno = e.depno)
		where e.depno is null
	 *
	 */
	using als_d = alias_d<Department>;
	using als_e = alias_e<Employee>;

	auto statement = storage.prepare(select(asterisk<als_d>(), from<als_d>(), left_join<als_e>(on
	(c(alias_column<als_d>(&Department::m_deptno)) == alias_column<als_e>(&Employee::m_deptno))), where(is_null(alias_column<als_e>(&Employee::m_deptno)))));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);
	auto& row = rows[0];
	//static_assert(std::is_same_v<decltype(row),std::tuple<>&>);	// BUG!
	// auto fld1 = std::get<0>(row);

	std::ignore = row;
}

void SQL3_6()
{
	/*
	 * select e.ename, d.loc from emp e, dept d where e.depno = d.deptno
	 *
	 *
	 */
	using als_d = alias_d<Department>;
	using als_e = alias_e<Employee>;

	auto statement = storage.prepare(select(columns(alias_column<als_e>(&Employee::m_ename), alias_column<als_d>(&Department::m_loc)),
		where(c(alias_column<als_e>(&Employee::m_deptno)) == alias_column<als_d>(&Department::m_deptno))));
	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);


	/*
	 * select e.ename, d.loc, b.received from emp e join dept d on (e.depno == d.deptno)
	 * left_join emp_bonus b on (e.empno = b.empno)
	 * order by d.loc
	 *
	 *
	 */
	using als_b = alias_b<EmpBonus>;

	{
		auto statement = storage.prepare(select(
			columns(alias_column<als_e>(&Employee::m_ename), alias_column<als_d>(&Department::m_loc), alias_column<als_b>(&EmpBonus::m_received)), from<als_e>(),
			join<als_d>(on(c(alias_column<als_e>(&Employee::m_deptno)) == alias_column<als_d>(&Department::m_deptno))),
			left_join<als_b>(on(c(alias_column<als_e>(&Employee::m_empno)) == alias_column<als_b>(&EmpBonus::m_empno))), order_by(alias_column<als_d>(&Department::m_loc))));
		auto sql = statement.expanded_sql();
		auto rows = storage.execute(statement);

	}

}

void SQL3_9()
{
// DYNAMIC FROM:
/*
select deptno, sum(salary) as total_sal, sum(bonus) as total_bonus from
(
select e.empno
, e.ename
, e.salary
, e.deptno
, b.type
, e.salary *
case 
when b.type = 1 then .1
when b.type = 2 then .2
else .3
end as bonus

from emp e, emp_bonus b
where e.empno = b.empno
and e.deptno =20
)
group by deptno
*/


	// INNER SELECT:

	/*
	 * select e.empno
, e.ename
, e.salary
, e.deptno
, b.type
, e.salary *
case 
when b.type = 1 then .1
when b.type = 2 then .2
else .3
end as bonus

from emp e, emp_bonus b
where e.empno = eb.empno
and e.deptno =20
	 *
	 */

	struct Bonus : alias_tag
	{
		static constexpr std::string get() { return "bonus"; }
	};
	using als_e = alias_e<Employee>;
	using als_b = alias_b<EmpBonus>;


	auto statement = storage.prepare(select(columns(
		alias_column<als_e>(&Employee::m_empno),
		alias_column<als_e>(&Employee::m_ename),
		alias_column<als_e>(&Employee::m_salary),
		alias_column<als_e>(&Employee::m_deptno),
		alias_column<als_b>(&EmpBonus::m_type),
		as<Bonus>(mul(alias_column<als_e>(&Employee::m_salary),
			case_<double>().
			when(c(alias_column<als_b>(&EmpBonus::m_type)) == 1, then(0.1)).
			when(c(alias_column<als_b>(&EmpBonus::m_type)) == 2, then(0.2)).else_(0.3).end()))),
		where(c(alias_column<als_e>(&Employee::m_empno)) == alias_column<als_b>(&EmpBonus::m_empno)
			and c(alias_column<als_e>(&Employee::m_deptno)) == 20)));

	auto sql = statement.expanded_sql();
	auto rows = storage.execute(statement);


}

void usingUpdate()
{
	storage.update_all(set(c(&Employee::m_salary) = c(&Employee::m_salary) * 1.3), where(c(&Employee::m_job) == "Clerk"));
	auto expression = update_all(set(c(&Employee::m_salary) = c(&Employee::m_salary) * 1.3), where(c(&Employee::m_job) == "Clerk"));
	std::string sql = storage.dump(expression);
	auto statement = storage.prepare(expression);
	storage.execute(statement);

	auto objects = storage.get_all<Employee>(where(c(&Employee::m_job) == "Clerk"));
}

void usingDelete()
{
	storage.remove_all<Employee>(where(c(&Employee::m_empno) == 5));
	auto expression = remove_all<Employee>(where(c(&Employee::m_empno) == 6));
	std::string sql = storage.dump(expression);
	auto statement = storage.prepare(expression);
	storage.execute(statement);
}

void usingObjectAPI()
{
	try
	{
		auto objects = storage.get_all<Employee>();		// SELECT * FROM EMP
		auto employee = storage.get<Employee>(7499);	// SELECT * FROM EMP WHERE id = 7499

		Employee emp{ -1, "JOSE", "ENG", std::nullopt, "17-DEC-1980", 32000, std::nullopt, 10 };
		emp.m_empno = storage.insert(emp);
		// INSERT INTO EMP ( 'ALL COLUMNS EXCEPT PRIMARY KEY COLUMNS' )
		// VALUES (	'VALUES TAKEN FROM emp OBJECT')


		emp.m_salary *= 1.3;
		storage.update(emp);
		//	UPDATE Emp
		//	SET
		//		column_name = emp.field_name	// for all columns except primary key columns
		//		// ....
		//	WHERE empno = emp.m_empno;

		storage.remove<Employee>(emp.m_empno);
		// DELETE FROM Emp WHERE empno = emp.m_empno

		storage.remove_all<Employee>(where(c(&Employee::m_salary) < 1000));
		// DELETE FROM Emp WHERE 'where clause'

		storage.remove_all<Employee>();
		// DELETE FROM Emp
	}
	catch(std::exception& ex)
	{
		auto s = ex.what();
		std::ignore = s;
	}
}

void SQL3_9a()
{
	// select  deptno, sum(salary) as total_sal, sum(bonus) as total_bonus from(
	// 	SELECT e.empno,
	// 	e.ename,
	// 	e.salary,
	// 	e.deptno,
	// 	sum(e.salary* case when eb.type = 1 then .1
	// 		when eb.type = 2 then  .2
	// 				   else .3
	// 				   end) as bonus
	// 	FROM Emp e, emp_bonus eb
	// 	where e.empno = eb.empno
	// 	and e.deptno = 10
	// 		group by e.empno)	// group emp_bonus by employee id


}

void SQL3_9b()
{
	// select d.deptno, d.total_sal, sum(e.salary * case
	// 	when eb.type = 1 then .1
	// 	when eb.type = 2 then .2
	// 										else .3
	// 										end) as total_bonus
	// 	from emp e, emp_bonus eb,
	// 	(
	// 	select deptno, sum(salary) as total_sal
	// 	from emp
	// 	where deptno = 10
	// 		group by deptno
	// 		) d
	// 		where e.deptno = d.deptno
	// 		and e.empno = eb.empno
	// 			group by d.deptno, d.total_sal
}

void SQL3_8c()
{
	// select d.deptno, d.total_sal, sum(e.salary * case
	// 	when eb.type = 1 then .1
	// 	when eb.type = 2 then .2
	// 										else .3
	// 										end) as total_bonus
	// 	from emp e, emp_bonus eb, dep_salary d
	// 	where e.deptno = d.deptno
	// 	and e.empno = eb.empno
	// 		group by d.deptno, d.total_sal
	//
	//
	// dep_salary view:
	//
	// 	select deptno, sum(salary) as total_sal
	// 		from emp
	// 	where deptno = 10
	// 		group by deptno

}