#pragma once

#include "../dev/functional/cxx_core_features.h"

// need support for #include <compare> for advanced std::chrono members
#if defined(SQLITE_ORM_THREE_WAY_COMPARISON_SUPPORTED)

#include <chrono>
#include <sstream>
#include <regex>
#include <format>

inline std::chrono::sys_days today() {
    const auto today = std::chrono::sys_days{floor<std::chrono::days>(std::chrono::system_clock::now())};
    return today;
}

/**
 *  This is the date we want to map to our sqlite db.
  *  Let's make it `TEXT`
  */

//  also we need transform functions to make string from enum..
inline std::string SysDaysToString(std::chrono::sys_days pt) {
    auto r = std::format("{:%F}", pt);
    return r;
}

constexpr auto null_sys_day = std::chrono::sys_days{std::chrono::days{0}};

/**
 *  and sys_days from string. This function has nullable result cause
 *  string can be neither `male` nor `female`. i have taken null_sys_day to
 *  correspond to null. Don't know if should use std::optional as return case??
 *  Of course we won't allow this to happen but as developers we must have
 *  a scenario for this case.
 *  These functions are just helpers. They will be called from several places
 *  that's why I placed it separatedly. You can use any transformation type/form
 *  (for example BETTER_ENUM https://github.com/aantron/better-enums)
 */
inline std::chrono::sys_days SysDaysFromString(const std::string& s) {
    using namespace std::literals;
    using namespace std::chrono;

    std::stringstream ss{s};
    std::chrono::sys_days tt;
    ss >> std::chrono::parse("%F"s, tt);
    if(!ss.fail()) {
        return tt;
    }
    return null_sys_day;
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
            return statement_binder<std::string>().bind(stmt, index, SysDaysToString(value));
        }
    };

    /**
	 *  field_printer is used in `dump` and `where` functions. Here we have to create
	 *  a string from mapped object.
	 */
    template<>
    struct field_printer<std::chrono::sys_days> {
        std::string operator()(const std::chrono::sys_days& t) const {
            return SysDaysToString(t);
        }
    };

    /**
	 *  This is a reverse operation: here we have to specify a way to transform string received from
	 *  database to our sysdays object. Here we call `SysDaysFromString` and throw `std::runtime_error` if it returns
	 *  null_sys_day. Every `row_extractor` specialization must have `extract(const char*)`, `extract(sqlite3_stmt *stmt, int columnIndex)`
	 *	and `extract(sqlite3_value* value)`
	 *  functions which return a mapped type value.
	 */
    template<>
    struct row_extractor<std::chrono::sys_days> {
        std::chrono::sys_days extract(const char* row_value) const {
            auto sd = SysDaysFromString(row_value);
            if(sd != null_sys_day) {
                return sd;
            } else {
                throw std::runtime_error("incorrect date string (" + std::string(row_value) + ")");
            }
        }

        std::chrono::sys_days extract(sqlite3_stmt* stmt, int columnIndex) const {
            auto str = sqlite3_column_text(stmt, columnIndex);
            return this->extract((const char*)str);
        }
        std::chrono::sys_days extract(sqlite3_value* row_value) const {
            auto characters = reinterpret_cast<const char*>(sqlite3_value_text(row_value));
            return extract(characters);
        }
    };
}

#endif