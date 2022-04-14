#pragma once

#include <sqlite_orm/sqlite_orm.h>
#include <optional>
#include <string>
#include <filesystem>
#include <stdio.h>
#include <fstream>

// #include "..\ORM_Extensions/SchemaManager.h"
#include <sqlite_orm/SchemaManager.h>

inline namespace v0
{
	struct Employee
	{
		int m_empno;
		std::string m_ename;
		std::string m_job;
		std::optional<int> m_mgr;
		std::string m_hiredate;
		double m_salary;
		std::optional<double> m_commission;
		int m_deptno;
	};

	struct Department
	{
		int m_deptno;
		std::string m_deptname;
		std::string m_loc;
		std::optional<int> m_manager;
	};

	struct EmpBonus
	{
		int m_id;
		int m_empno;
		std::string m_received;	// date
		int m_type;
	};

	struct Artist
	{
		int m_id;
		std::string m_name;
	};

	struct Album
	{
		int m_id;
		int m_artist_id;
		int m_year;
		int m_age;
	};

	using namespace sqlite_orm;


	inline auto create_storage(std::string dbFileName)
	{
		auto storage = make_storage(dbFileName,
			make_table("Emp",
				make_column("empno", &Employee::m_empno, primary_key(), autoincrement()),
				make_column("ename", &Employee::m_ename, default_value("?"),col_changed()),
				make_column("job", &Employee::m_job),
				make_column("mgr", &Employee::m_mgr),
				make_column("hiredate", &Employee::m_hiredate),
				make_column("salary", &Employee::m_salary,default_value(100)),
				make_column("comm", &Employee::m_commission),
				make_column("deptno", &Employee::m_deptno),
				foreign_key(&Employee::m_deptno).references(&Department::m_deptno),
				check(c(&Employee::m_salary) > coalesce<double>(&Employee::m_commission, 0.0))),
			make_table("Dept",
				make_column("deptno", &Department::m_deptno, primary_key(), autoincrement()),
				make_column("deptname", &Department::m_deptname),
				make_column("loc", &Department::m_loc),
				make_column("mgr", &Department::m_manager,col_changed()),
				foreign_key(&Department::m_manager).references(&Employee::m_empno)),
			make_table("Emp_bonus",
				make_column("id", &EmpBonus::m_id, primary_key(), autoincrement()),
				make_column("empno", &EmpBonus::m_empno),
				make_column("received", &EmpBonus::m_received),
				make_column("type", &EmpBonus::m_type),
				foreign_key(&EmpBonus::m_empno).references(&Employee::m_empno)),
			make_table("Artists",
				make_column("id", &Artist::m_id, primary_key(), autoincrement()),
				make_column("name", &Artist::m_name)),
			make_table("Albums",
				make_column("id", &Album::m_id, primary_key(), autoincrement()),
				make_column("artist_id", &Album::m_artist_id),
				make_column("year", &Album::m_year),
				make_column("age", &Album::m_age),                 //, generated_always_as(c(&Album::m_year) + 20)),
				// check(c(&Album::m_year) > 0),
				foreign_key(&Album::m_artist_id).references(&Artist::m_id)));
		auto ver = storage.pragma.user_version();
		storage.pragma.user_version(0);

		// if (sync) {
		// 	storage.sync_schema();	// enable it in a normal day to day application; call it specifically when col_changed() applies to at least one table
		// }
		return storage;
	}

	inline auto storage = create_storage("SQLCookbook.sqlite");
}

namespace Versioning
{
	using namespace std::literals;

	// inline int user_version = 1;
	inline void migrate0to1() {}
	inline void migrate1to2() {}
	inline void migrate2to3() {}
	inline std::vector < std::function<void()>> schemas{ migrate0to1, migrate1to2, migrate2to3 };

	template<typename Storage>
	struct Migration {
		Storage& storage;
		int m_current_version;
		int m_application_version = 2;
		Migration(Storage& storage) {
			m_current_version = storage.pragma.user_version();

		}
	};
}
/*
 * Employee -> Department
 * EmpBonus -> Employee
 * Album -> Artist
 *
 *
 *	INSERT ORDER:
 *
 * 1- Department
 * 2- Employee
 * 3- EmpBonus
 * 4- Artist
 * 5- Album
 *
 *	DROP ORDER
 *
 *	1- Album
 *	2- Artist
 *	3- EmpBonus
 *	4- Employee
 *	5- Department
 *
 */

