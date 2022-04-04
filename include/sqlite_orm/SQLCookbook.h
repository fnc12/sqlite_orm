#pragma once

#include <sqlite_orm/sqlite_orm.h>
#include <optional>
#include <string>

// #include "..\ORM_Extensions/SchemaManager.h"
#include <sqlite_orm/SchemaManager.h>


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
};

using namespace sqlite_orm;

auto storage = make_storage("SQLCookbook.sqlite",
	make_table("Emp",
		make_column("empno", &Employee::m_empno, primary_key(), autoincrement()),
		make_column("ename", &Employee::m_ename),
		make_column("job", &Employee::m_job),
		make_column("mgr", &Employee::m_mgr),
		make_column("hiredate", &Employee::m_hiredate),
		make_column("salary", &Employee::m_salary),
		make_column("comm", &Employee::m_commission),
		make_column("deptno", &Employee::m_deptno),
		foreign_key(&Employee::m_deptno).references(&Department::m_deptno),
		check(c(&Employee::m_salary) > &Employee::m_commission)),
	make_table("Dept",
		make_column("deptno", &Department::m_deptno, primary_key(), autoincrement()),
		make_column("deptname", &Department::m_deptname),
		make_column("loc", &Department::m_loc),
		make_column("mgr", &Department::m_manager),
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
		foreign_key(&Album::m_artist_id).references(&Artist::m_id)));


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

