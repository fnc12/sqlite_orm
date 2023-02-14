/****
 Implemented example from here https://www.tutorialspoint.com/sqlite/sqlite_select_query.htm.
 */

#include <sqlite_orm/sqlite_orm.h>

#include <iostream>

using namespace sqlite_orm;
using std::cout;
using std::endl;
using std::make_unique;
using std::string;

void all_employees() {
    struct Employee {
        int id;
        std::string name;
        int age;
        std::unique_ptr<std::string> address;  //  optional
        std::unique_ptr<double> salary;  //  optional
    };

    auto storage = make_storage("select.sqlite",
                                make_table("COMPANY",
                                           make_column("ID", &Employee::id, primary_key()),
                                           make_column("NAME", &Employee::name),
                                           make_column("AGE", &Employee::age),
                                           make_column("ADDRESS", &Employee::address),
                                           make_column("SALARY", &Employee::salary)));
    storage.sync_schema();
    storage.remove_all<Employee>();  //  remove all old employees in case they exist in db..

    //  create employees..
    Employee paul{-1, "Paul", 32, make_unique<string>("California"), make_unique<double>(20000.0)};
    Employee allen{-1, "Allen", 25, make_unique<string>("Texas"), make_unique<double>(15000.0)};
    Employee teddy{-1, "Teddy", 23, make_unique<string>("Norway"), make_unique<double>(20000.0)};
    Employee mark{-1, "Mark", 25, make_unique<string>("Rich-Mond"), make_unique<double>(65000.0)};
    Employee david{-1, "David", 27, make_unique<string>("Texas"), make_unique<double>(85000.0)};
    Employee kim{-1, "Kim", 22, make_unique<string>("South-Hall"), make_unique<double>(45000.0)};
    Employee james{-1, "James", 24, make_unique<string>("Houston"), make_unique<double>(10000.0)};

    //  insert employees. `insert` function returns id of inserted object..
    paul.id = storage.insert(paul);
    allen.id = storage.insert(allen);
    teddy.id = storage.insert(teddy);
    mark.id = storage.insert(mark);
    david.id = storage.insert(david);
    kim.id = storage.insert(kim);
    james.id = storage.insert(james);

    //  print users..
    cout << "paul = " << storage.dump(paul) << endl;
    cout << "allen = " << storage.dump(allen) << endl;
    cout << "teddy = " << storage.dump(teddy) << endl;
    cout << "mark = " << storage.dump(mark) << endl;
    cout << "david = " << storage.dump(david) << endl;
    cout << "kim = " << storage.dump(kim) << endl;
    cout << "james = " << storage.dump(james) << endl;

    //  select all employees..
    auto allEmployees = storage.get_all<Employee>();

    cout << "allEmployees[0] = " << storage.dump(allEmployees[0]) << endl;
    cout << "allEmployees count = " << allEmployees.size() << endl;

    //  now let's select id, name and salary..
    auto idsNamesSalarys = storage.select(columns(&Employee::id, &Employee::name, &Employee::salary));
    for(auto& row: idsNamesSalarys) {  //  row's type is tuple<int, string, unique_ptr<double>>
        cout << "id = " << get<0>(row) << ", name = " << get<1>(row) << ", salary = ";
        if(get<2>(row)) {
            cout << *get<2>(row);
        } else {
            cout << "null";
        }
        cout << endl;
    }

    cout << endl;

    auto allEmployeeTuples = storage.select(asterisk<Employee>());
    cout << "allEmployeeTuples count = " << allEmployeeTuples.size() << endl;
    for(auto& row: allEmployeeTuples) {  //  row's type is std::tuple<int, string, int, std::unique_ptr<string>,
        //  std::unique_ptr<double>>
        cout << get<0>(row) << '\t' << get<1>(row) << '\t' << get<2>(row) << '\t';
        if(auto& value = get<3>(row)) {
            cout << *value;
        } else {
            cout << "null";
        }
        cout << '\t';
        if(auto& value = get<4>(row)) {
            cout << *value;
        } else {
            cout << "null";
        }
        cout << '\t' << endl;
    }

    cout << endl;

    auto allEmployeeObjects = storage.select(object<Employee>());
    cout << "allEmployeeObjects count = " << allEmployeeObjects.size() << endl;
    for(auto& employee: allEmployeeObjects) {
        cout << employee.id << '\t' << employee.name << '\t' << employee.age << '\t';
        if(auto& value = employee.address) {
            cout << *value;
        } else {
            cout << "null";
        }
        cout << '\t';
        if(auto& value = employee.salary) {
            cout << *value;
        } else {
            cout << "null";
        }
        cout << '\t' << endl;
    }

    cout << endl;
}

void all_artists() {
    struct Artist {
        int id;
        std::string name;
    };

    struct Album {
        int id;
        int artist_id;
    };

    auto storage = make_storage("",
                                make_table("artists",
                                           make_column("id", &Artist::id, primary_key().autoincrement()),
                                           make_column("name", &Artist::name)),
                                make_table("albums",
                                           make_column("id", &Album::id, primary_key().autoincrement()),
                                           make_column("artist_id", &Album::artist_id),
                                           foreign_key(&Album::artist_id).references(&Artist::id)));
    storage.sync_schema();
    storage.transaction([&storage] {
        auto artistPk = storage.insert(Artist{-1, "Artist"});
        storage.insert(Album{-1, artistPk});
        storage.insert(Album{-1, artistPk});
        return true;
    });

    // SELECT artists.*, albums.* FROM artists JOIN albums ON albums.artist_id = artist.id

    cout << "artists.*, albums.*\n";
    // row's type is std::tuple<int, std::string, id, int>
    for(auto& row: storage.select(columns(asterisk<Artist>(), asterisk<Album>()),
                                  join<Album>(on(c(&Album::artist_id) == &Artist::id)))) {
        cout << get<0>(row) << '\t' << get<1>(row) << '\t' << get<2>(row) << '\t' << get<3>(row) << '\n';
    }
    cout << endl;
}

int main() {

    try {
        all_employees();
        all_artists();
    } catch(const std::system_error& e) {
        cout << "[" << e.code() << "] " << e.what();
    }

    return 0;
}
