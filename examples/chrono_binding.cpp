#include <sqlite_orm/sqlite_orm.h>
#include <cassert>
#include <string>
#include <iostream>

#if defined(SQLITE_ORM_THREE_WAY_COMPARISON_SUPPORTED)

#include "date_binding.h"

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
    using namespace std::chrono_literals;

    year_month_day birthdate{year{1960}, month{7}, day{26}};
    year_month_day or_birthdate{1960y, July, 26d};
    assert(birthdate == or_birthdate);

    sys_days birthdate_tp = birthdate;  // auto conversion to time_point of days units

    storage.sync_schema();

    Person person{1, "Juan Dent", birthdate};  // we are using the implicit operator from year_month_day to sys_days
    storage.replace(person);

    auto pers = storage.get<Person>(1);
    year_month_day ymd = pers.birthdate;

    {
        constexpr sys_days thursday = year_month_weekday{year{2022} / March / Thursday[2]};
        constexpr year_month_day second_thursday = thursday;
        auto diff = (thursday - birthdate_tp).count();
        std::cout << "Number of days " << diff << " ";
        auto year_diff = duration_cast<years>(thursday - birthdate_tp).count();
        std::cout << "Number of years " << year_diff << std::endl;
        std::ignore = thursday;
    }
    {
        constexpr sys_days thursday = year_month_weekday{year{2022} / March / Thursday[last]};
        constexpr year_month_day last_thursday = thursday;
        std::ignore = last_thursday;
    }

    {
        constexpr sys_days monday = year_month_weekday_last{2022y, month{March}, weekday_last{Monday}};
        constexpr year_month_day last_monday = monday;
        std::ignore = monday;
    }

    assert(ymd == birthdate);
}

#endif
