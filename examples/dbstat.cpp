/**
 *  This example demonstrates the DBSTAT virtual table.
 *
 *  The eponymous virtual table `dbstat` reports the pages of the b-trees in a database file - the tables and indexes,
 *  and the schema table `sqlite_schema` itself - with their size, number of cells, payload and unused bytes.
 *  Its hidden columns 'schema' and 'aggregate' are accessible through `dbstat::hidden`, e.g. `dbstat::hidden::aggregate`
 *  to report one row per b-tree instead of one per page.
 *
 *  The DBSTAT virtual table is available only if SQLite is built with SQLITE_ENABLE_DBSTAT_VTAB,
 *  which also has to be defined when compiling sqlite_orm - by the user, or by a package manager
 *  like vcpkg in its configuration of sqlite3.h.
 *  It is added to the storage along with the other tables SQLite provides itself, like the schema table,
 *  by `make_builtin_tables()`.
 *
 *  For information about the DBSTAT virtual table see https://sqlite.org/dbstat.html
 */

#include <sqlite_orm/sqlite_orm.h>

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
#include <iostream>
#include <string>

using namespace sqlite_orm;
using std::cout;
using std::endl;

struct Note {
    int64 id = 0;
    std::string title;
    std::string text;
};
#endif  //  ENABLE_THIS_EXAMPLE

int main() {
#ifdef ENABLE_THIS_EXAMPLE
    auto storage = make_storage(":memory:",
                                make_builtin_tables(),
                                make_index("idx_notes_title", &Note::title),
                                make_table("notes",
                                           make_column("id", &Note::id, primary_key()),
                                           make_column("title", &Note::title),
                                           make_column("text", &Note::text)));
    storage.sync_schema();

    storage.transaction([&storage] {
        for (int i = 0; i < 1000; ++i) {
            storage.insert(Note{0, "note #" + std::to_string(i), std::string(200, char('a' + i % 26))});
        }
        return true;
    });

    //  SELECT "dbstat"."name", COUNT(*), TOTAL("dbstat"."pgsize") FROM "dbstat"
    //  GROUP BY "dbstat"."name" ORDER BY "dbstat"."name"
    cout << "Pages per b-tree:" << endl;
    for (auto& [name, pages, bytes]: storage.select(columns(&dbstat::name, count(), total(&dbstat::pgsize)),
                                                    group_by(&dbstat::name),
                                                    order_by(&dbstat::name))) {
        cout << "  " << name << ": " << pages << " pages, " << int64(bytes) << " bytes" << endl;
    }
    cout << endl;

    //  The kinds of pages of the table 'notes', the internal and leaf pages of its b-tree:
    //  SELECT "dbstat"."pagetype", COUNT(*), TOTAL("dbstat"."ncell") FROM "dbstat"
    //  WHERE ("dbstat"."name" = 'notes') GROUP BY "dbstat"."pagetype"
    cout << "Page types of 'notes':" << endl;
    for (auto& [pagetype, pages, cells]: storage.select(columns(&dbstat::pagetype, count(), total(&dbstat::ncell)),
                                                        where(c(&dbstat::name) == "notes"),
                                                        group_by(&dbstat::pagetype))) {
        cout << "  " << pagetype << ": " << pages << " pages, " << int64(cells) << " cells" << endl;
    }
    cout << endl;

#if SQLITE_VERSION_NUMBER >= 3031000
    //  With the hidden column 'aggregate', there is one row per b-tree, with the column 'pageno' holding
    //  the number of pages, and the other columns their totals:
    //  SELECT "dbstat"."name", "dbstat"."pageno", "dbstat"."payload", "dbstat"."unused" FROM "dbstat"
    //  WHERE ("aggregate" = 1) ORDER BY "dbstat"."name"
    cout << "Payload and unused bytes per b-tree:" << endl;
    for (auto& [name, pages, payload, unused]:
         storage.select(columns(&dbstat::name, &dbstat::pageno, &dbstat::payload, &dbstat::unused),
                        where(c(&dbstat::hidden::aggregate) == true),
                        order_by(&dbstat::name))) {
        cout << "  " << name << ": " << pages << " pages, " << payload << " bytes payload, " << unused
             << " bytes unused" << endl;
    }
    cout << endl;
#endif

    //  Joined with the schema table, the b-trees are told apart by the kind of database object:
    //  SELECT "sqlite_master"."type", "sqlite_master"."name", COUNT(*) FROM "dbstat", "sqlite_master"
    //  WHERE ("sqlite_master"."name" = "dbstat"."name")
    //  GROUP BY "sqlite_master"."type", "sqlite_master"."name" ORDER BY "sqlite_master"."type", "sqlite_master"."name"
    cout << "Pages per table and index:" << endl;
    for (auto& [type, name, pages]:
         storage.select(columns(&sqlite_master::type, &sqlite_master::name, count()),
                        where(c(&sqlite_master::name) == &dbstat::name),
                        group_by(&sqlite_master::type, &sqlite_master::name),
                        multi_order_by(order_by(&sqlite_master::type), order_by(&sqlite_master::name)))) {
        cout << "  " << type << ' ' << name << ": " << pages << " pages" << endl;
    }
    cout << endl;
#endif  //  ENABLE_THIS_EXAMPLE
    return 0;
}
