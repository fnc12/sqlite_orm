#pragma once

#include <fstream>
#include <chrono>
#include <ctime>  // std::localtime and std::tm and std::time_t
#include <iomanip>  // std::put_time
#include <memory>
#include <string>
#include <memory>
#include "error_code.h"

enum class auditing_behavior : signed char { OFF = 0, ON = 1 };

namespace sqlite_orm {
    namespace internal {
        class storage_base;
    }
}
using sqlite_orm::internal::storage_base;

struct log_settings {
    auditing_behavior behavior;
    std::shared_ptr<std::ofstream> log_file;
    std::string log_file_name;
    std::string time_format;
    enum status_code { FAIL, GOOD } status = status_code::FAIL;
    bool global_settings = false;

  public:
    log_settings(auditing_behavior behave,
                 std::shared_ptr<std::ofstream> file,
                 std::string file_name,
                 std::string format,
                 status_code stats,
                 bool glob_settings) {
        behavior = behave;
        log_file = file;
        bool ok = log_file->good();
        bool open = log_file->is_open();
        log_file_name = file_name;
        time_format = format;
        status = stats;
        global_settings = glob_settings;
    }
    log_settings() {}
    void open() {
        using namespace std;
        if(!log_file->is_open()) {
            log_file->open(log_file_name, ios::trunc | ios::out);
            if(!log_file->good()) {
                status = FAIL;
                log_file->close();
                throw std::system_error{sqlite_orm::orm_error_code::failure_to_init_logfile};
            }
        }
        status = GOOD;
    }
    void log(std::string message) {
        if(status == GOOD && behavior == auditing_behavior::ON) {
            // would use format if C++ 20
            auto now = std::chrono::system_clock::now();

            std::time_t now_time = std::chrono::system_clock::to_time_t(now);

            // Convert to local time (std::tm structure)
            // WARNING: localtime is not thread safe!
            std::tm local_time = *std::localtime(&now_time);

            // Print the local time in a human-readable format
            *log_file << "@: " << std::put_time(&local_time, time_format.c_str()) << " = ";
            *log_file << message << std::endl;
        }
    }
};

inline std::shared_ptr<std::ofstream> get_file(std::string log_file_name) {
    static std::map<std::string, std::shared_ptr<std::ofstream>> files;
    auto it = files.find(log_file_name);
    if(it == files.end()) {
        std::shared_ptr<std::ofstream> pointer_to_shared{new std::ofstream()};
        files.insert({log_file_name, pointer_to_shared});
    }
    return files[log_file_name];
}

struct log_admin {
  private:
    log_admin(){};
    log_settings global_settings;  // not belonging to any storage_base!
    std::map<storage_base*, log_settings> settings;
    std::map<sqlite3*, storage_base*> map_sqlite3;
    bool global_settings_initialized = false;

  public:
    static log_admin& admin() {
        static log_admin admin{};
        return admin;
    }

    void register_db(storage_base* sbase, sqlite3* db) {
        map_sqlite3[db] = sbase;
    }

    // pass nullptr if not belonging to one particular storage_base!
    void open_log_file(std::string log_file_name,
                       auditing_behavior on_or_off,
                       storage_base* sbase,
                       std::string time_format) {
        std::shared_ptr<std::ofstream> log_file = get_file(log_file_name);
        if(sbase == nullptr) {
            global_settings = log_settings(on_or_off, log_file, log_file_name, time_format, log_settings::FAIL, false);
            global_settings.open();
            global_settings_initialized = true;
        } else {
            settings[sbase] = log_settings(on_or_off, log_file, log_file_name, time_format, log_settings::FAIL, false);
            settings[sbase].open();
        }
    }
    void set_auditing_behavior(auditing_behavior on_or_off, storage_base* sbase) {
        auto pointer = settings.find(sbase);
        if(pointer != settings.end()) {
            pointer->second.behavior = on_or_off;
        }
    }
    void setFormatStr(std::string format, storage_base* sbase) {
        auto pointer = settings.find(sbase);
        if(pointer != settings.end()) {
            pointer->second.time_format = format;
        }
    }
    void log(const std::string& message, sqlite3* db) {
        storage_base* base = map_sqlite3[db];
        if(base) {
            auto pointer = settings.find(base);
            if(pointer != settings.end()) {
                pointer->second.log(message);
                return;
            }
        }
        // exclusive or behavior!!
        // not belonging to any particular storage_base
        if(global_settings_initialized) {
            global_settings.log(message);
        }
    }
};
