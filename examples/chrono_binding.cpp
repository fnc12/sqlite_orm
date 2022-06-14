#include <sqlite_orm/sqlite_orm.h>
#include <cassert>
#include <string>
#include <iostream>

#if defined(SQLITE_ORM_THREE_WAY_COMPARISON_SUPPORTED)

#include "..\dev/date_binding.h"

using namespace sqlite_orm;
using std::cout;
using std::endl;

// "sys_days_as_bindable_column_type"

struct Person {
    int id;
    std::string name;
    std::chrono::sys_days birthdate;
};

int main(int argc, const char* argv[]) {
    cout << argv[0] << endl;

    const std::string db_name = "sys_days.sqlite";
    ::remove(db_name.c_str());

    auto storage = make_storage(db_name,
                                make_table("Persons",
                                           make_column("id", &Person::id, primary_key(), autoincrement()),
                                           make_column("name", &Person::name),
                                           make_column("birthdate", &Person::birthdate)));

    using namespace std::chrono;

    year_month_day birthdate{year{1960}, month{7}, day{26}};

    storage.sync_schema();

    Person person{1, "Juan Dent", birthdate};
    storage.replace(person);

    auto pers = storage.get<Person>(1);
    year_month_day ymd = pers.birthdate;

    assert(ymd == birthdate);
}

#endif
