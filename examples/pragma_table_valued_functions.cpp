/**
 *  This example demonstrates PRAGMA table-valued functions.
 *
 *  A PRAGMA that returns results is also available as an eponymous virtual table named `pragma_<name>`,
 *  usable as a table-valued function: its result rows can be filtered, projected, ordered, and joined with
 *  other tables - e.g. with the schema table to list the columns of all tables at once.
 *  The PRAGMA argument and the schema are the hidden columns 'arg' and 'schema', accessible through the
 *  row type's nested `hidden` struct, e.g. `table_info::hidden::arg`.
 *
 *  sqlite_orm provides `pragma_table_info` and `pragma_table_xinfo` with C++26 reflection.
 *  Their eponymous virtual tables are added to the storage by `make_pragma_tables()`, or along with
 *  the other tables SQLite provides itself, like the schema table, by `make_builtin_tables()`.
 *  The application's tables are mapped by the reflection-based `make_table<T>()`, with the table names and
 *  column constraints given as annotations.
 *
 *  For information about PRAGMA functions see https://sqlite.org/pragma.html#pragfunc
 */

#include <sqlite_orm/sqlite_orm.h>

#if SQLITE_VERSION_NUMBER >= 3031000 && defined(SQLITE_ORM_REFLECTION_SUPPORTED)
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
#include <iostream>
#include <string>

using namespace sqlite_orm;
using std::cout;
using std::endl;

struct[[= "departments"_orm_name]] Department {
    [[= primary_key()]] int64 id = 0;
    [[= unique()]] std::string name;
};

struct[[= "products"_orm_name]] Product {
    [[= primary_key()]] int64 id = 0;
    std::string name;
    [[= default_value(0)]] int quantity = 0;
    double price = 0;
    [[= generated_always_as(&Product::price * c(&Product::quantity))]] double total_value = 0;
    int64 department_id = 0;
};
#endif  //  ENABLE_THIS_EXAMPLE

int main() {
#ifdef ENABLE_THIS_EXAMPLE
    auto storage = make_storage("",
                                make_builtin_tables(),
                                make_table<Department>(),
                                make_table<Product>(foreign_key(&Product::department_id).references(&Department::id)));
    storage.sync_schema();

    //  SELECT "pragma_table_info".* FROM "pragma_table_info"('products')
    cout << "Columns of 'products' (generated columns are not listed by pragma_table_info):" << endl;
    for (const table_info& column: storage.select(object<table_info>(), from(pragma_table_info_table("products")))) {
        cout << "  " << column.cid << ' ' << column.name << ' ' << column.type << (column.notnull ? " NOT NULL" : "")
             << (column.dflt_value.empty() ? "" : " DEFAULT " + column.dflt_value) << (column.pk ? " PRIMARY KEY" : "")
             << endl;
    }
    cout << endl;

    //  The table name can also be given as a constraint on the hidden column 'arg':
    //  SELECT "pragma_table_info"."name" FROM "pragma_table_info"
    //  WHERE (("pragma_table_info"."arg" = 'departments') AND ("pragma_table_info"."pk" > 0))
    cout << "Primary key columns of 'departments':" << endl;
    for (auto& name: storage.select(&table_info::name,
                                    where(c(&table_info::hidden::arg) == "departments" and c(&table_info::pk) > 0))) {
        cout << "  " << name << endl;
    }
    cout << endl;

    //  pragma_table_xinfo also lists generated columns and hidden columns of virtual tables,
    //  with the kind of column in its column 'hidden':
    //  SELECT "pragma_table_xinfo"."name", "pragma_table_xinfo"."hidden"
    //  FROM "pragma_table_xinfo"('products', 'main')
    //  WHERE ("pragma_table_xinfo"."hidden" != 0)
    cout << "Generated columns of 'products':" << endl;
    for (auto& [name, hidden]: storage.select(columns(&table_xinfo::name, &table_xinfo::hidden),
                                              from(pragma_table_xinfo_table("products", "main")),
                                              where(c(&table_xinfo::hidden) != 0))) {
        cout << "  " << name << (hidden == 2 ? " (VIRTUAL)" : hidden == 3 ? " (STORED)" : "") << endl;
    }
    cout << endl;

    //  Joined with the schema table, the table-valued function takes its argument from each schema row:
    //  SELECT "sqlite_master"."name", "pragma_table_info"."name", "pragma_table_info"."type"
    //  FROM "sqlite_master", "pragma_table_info"("sqlite_master"."name")
    //  WHERE ("sqlite_master"."type" = 'table')
    //  ORDER BY "sqlite_master"."name", "pragma_table_info"."cid"
    cout << "Columns of all tables:" << endl;
    for (auto& [tableName, columnName, type]:
         storage.select(columns(&sqlite_master::name, &table_info::name, &table_info::type),
                        from(sqlite_master_table, pragma_table_info_table(&sqlite_master::name)),
                        where(c(&sqlite_master::type) == "table"),
                        multi_order_by(order_by(&sqlite_master::name), order_by(&table_info::cid)))) {
        cout << "  " << tableName << '.' << columnName << ' ' << type << endl;
    }
    cout << endl;
#endif  //  ENABLE_THIS_EXAMPLE
    return 0;
}
