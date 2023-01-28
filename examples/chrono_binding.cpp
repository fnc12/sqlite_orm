#include <sqlite_orm/sqlite_orm.h>
#if __cpp_lib_chrono >= 201907L && __cpp_lib_format >= 201907L
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
#include <cassert>
#include <string>
#include <iostream>
#include <chrono>
#include <format>

///////////////////////////////
/// sys_days binding as TEXT
/// ///////////////////////////

#include <sstream>
#include <regex>

/**
 *  This is the date we want to map to our sqlite db.
 *  Let's make it `TEXT`
 */

//  also we need transform functions to make string from enum..
static std::string sysDaysToString(std::chrono::sys_days pt) {
    auto r = std::format("{:%F}", pt);
    return r;
}

/**
 *  and sys_days from string. This function has nullable result cause
 *  string can be an invalid sys_days. 
 *  Of course we won't allow this to happen but as developers we must have
 *  a scenario for this case.
 *  These functions are just helpers. They will be called from several places
 *  that's why I placed it separatedly. You can use any transformation type/form
 *  (for example BETTER_ENUM https://github.com/aantron/better-enums)
 */
static std::optional<std::chrono::sys_days> sysDaysFromString(const std::string& s) {
    using namespace std::literals;
    using namespace std::chrono;

    std::stringstream ss{s};
    std::chrono::sys_days tt;
    ss >> std::chrono::parse("%F"s, tt);
    if(!ss.fail()) {
        return {tt};
    }
    return std::nullopt;
}

/**
 *  This is where magic happens. To tell sqlite_orm how to act
 *  with SysDays we have to create a few service classes
 *  specializations (traits) in sqlite_orm namespace.
 */
namespace sqlite_orm {

    /**
     *  First of all is a type_printer template class.
     *  It is responsible for sqlite type string representation.
     *  We want SysDays to be `TEXT` so let's just derive from
     *  text_printer. Also there are other printers: real_printer and
     *  integer_printer. We must use them if we want to map our type to `REAL` (double/float)
     *  or `INTEGER` (int/long/short etc) respectively.
     */
    template<>
    struct type_printer<std::chrono::sys_days> : public text_printer {};

    /**
     *  This is a binder class. It is used to bind c++ values to sqlite queries.
     *  Here we have to create sysday string representation and bind it as string.
     *  Any statement_binder specialization must have `int bind(sqlite3_stmt*, int, const T&)` function
     *  which returns bind result. Also you can call any of `sqlite3_bind_*` functions directly.
     *  More here https://www.sqlite.org/c3ref/bind_blob.html
     */
    template<>
    struct statement_binder<std::chrono::sys_days> {

        int bind(sqlite3_stmt* stmt, int index, const std::chrono::sys_days& value) const {
            return statement_binder<std::string>().bind(stmt, index, sysDaysToString(value));
        }
    };

    /**
     *  field_printer is used in `dump` and `where` functions. Here we have to create
     *  a string from mapped object.
     */
    template<>
    struct field_printer<std::chrono::sys_days> {
        std::string operator()(const std::chrono::sys_days& t) const {
            return sysDaysToString(t);
        }
    };

    /**
     *  This is a reverse operation: here we have to specify a way to transform string received from
     *  database to our sysdays object. Here we call `sysDaysFromString` and throw `std::runtime_error` if it returns null.
     *  Every `row_extractor` specialization must have `extract(const char*)`, `extract(sqlite3_stmt *stmt, int columnIndex)`
     *	and `extract(sqlite3_value* value)`
     *  functions which return a mapped type value.
     */
    template<>
    struct row_extractor<std::chrono::sys_days> {
        std::chrono::sys_days extract(const char* row_value) const {
            if(row_value) {
                auto sd = sysDaysFromString(row_value);
                if(sd) {
                    return sd.value();
                } else {
                    throw std::runtime_error("incorrect date string (" + std::string(row_value) + ")");
                }
            } else {
                // ! row_value
                throw std::runtime_error("incorrect date string (nullptr)");
            }
        }

        std::chrono::sys_days extract(sqlite3_stmt* stmt, int columnIndex) const {
            auto str = sqlite3_column_text(stmt, columnIndex);
            return this->extract((const char*)str);
        }
        std::chrono::sys_days extract(sqlite3_value* row_value) const {
            auto characters = (const char*)(sqlite3_value_text(row_value));
            return extract(characters);
        }
    };
}

////////////////////////////////
/// end sys_days binding as TEXT
////////////////////////////////

using namespace sqlite_orm;
using std::cout;
using std::endl;

struct Person {
    int id;
    std::string name;
    std::chrono::sys_days birthdate;
};
#endif

int main(int, char**) {
#ifdef ENABLE_THIS_EXAMPLE
    const std::string db_name = "sys_days.sqlite";
    ::remove(db_name.c_str());

    auto storage = make_storage(db_name,
                                make_table("Persons",
                                           make_column("id", &Person::id, primary_key().autoincrement()),
                                           make_column("name", &Person::name),
                                           make_column("birthdate", &Person::birthdate)));

    using namespace std::chrono;

    year_month_day birthdate{year{1960}, month{7}, day{26}};

    storage.sync_schema();

    Person person{1, "Juan Dent", birthdate};  // we are using the implicit operator from year_month_day to sys_days
    storage.replace(person);

    auto pers = storage.get<Person>(1);
    year_month_day ymd = pers.birthdate;  // using the implicit operator from sys_days to year_month_day
    assert(ymd == birthdate);
#endif
    return 0;
}
