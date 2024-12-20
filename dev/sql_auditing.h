#pragma once
#include <fstream>
#include "error_code.h"
#include <chrono>
#include <ctime>  // For std::localtime and std::tm
#include <iomanip>  // For std::put_time
#include <string>

enum class auditing_behavior : signed char { OFF = 0, ON = 1 };

class sql_auditor_settings;

class sql_auditor {

    std::ofstream log_file;
    friend class sql_auditor_settings;
    sql_auditor();
    void open();
    inline static sql_auditor& auditor() {
        static sql_auditor auditor{};
        return auditor;
    }

  public:
    static void log(const std::string& message);
};

class sql_auditor_settings {
    std::string destination_file = "sql_auditor.txt";
    friend class sql_auditor;
    sql_auditor_settings() {}
    inline static sql_auditor_settings& settings() {
        static sql_auditor_settings sql_settings;
        return sql_settings;
    }
    auditing_behavior behavior = auditing_behavior::ON;
    std::string format_str = "%Y-%m-%d %H:%M:%S";

  public:
    static void set_destination_file(const std::string& filename);
    static void set_behavior(auditing_behavior behavior) {
        settings().behavior = behavior;
    }
    static void set_format(const std::string& format_str) {
        settings().format_str = format_str;
    }
};

inline sql_auditor::sql_auditor() {
    open();
}

inline void sql_auditor_settings::set_destination_file(const std::string& filename) {
    settings().destination_file = filename;
    sql_auditor::auditor().open();
}

inline void sql_auditor::open() {
    using namespace std;
    if(!log_file.is_open()) {
        log_file.open(sql_auditor_settings::settings().destination_file, ios::trunc | ios::out);
        if(!log_file.good()) {
            throw std::system_error{sqlite_orm::orm_error_code::failure_to_init_logfile};
        }
    }
}

inline void sql_auditor::log(const std::string& message) {
    // guard and exit if off
    if(sql_auditor_settings::settings().behavior == auditing_behavior::OFF)
        return;

    // would use format if C++ 20
    auto now = std::chrono::system_clock::now();

    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // Convert to local time (std::tm structure)
    // WARNING: localtime is not thread safe!
    std::tm local_time = *std::localtime(&now_time);

    // Print the local time in a human-readable format
    auditor().log_file << "@: " << std::put_time(&local_time, sql_auditor_settings::settings().format_str.c_str())
                       << " = ";
    auditor().log_file << message << std::endl;
}
