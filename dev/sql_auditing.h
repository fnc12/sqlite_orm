#pragma once
#include <fstream>
#include "error_code.h"
#include <chrono>
#include <ctime>  // For std::localtime and std::tm
#include <iomanip>  // For std::put_time
#include <string>

class sql_auditor {
    std::ofstream log_file;
    sql_auditor(std::string path) {
        using namespace std;
        log_file.open(path, ios::trunc | ios::out);
        if(!log_file.good()) {
            throw std::system_error{sqlite_orm::orm_error_code::failure_to_init_logfile};
        }
    }
    inline static sql_auditor& auditor() {
        static sql_auditor auditor{"sql_auditor.txt"};
        return auditor;
    }

  public:
    static void log(const std::string& message) {
        // would use format if C++ 20
        auto now = std::chrono::system_clock::now();

        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        // Convert to local time (std::tm structure)
        // WARNING: localtime is not thread safe!
        std::tm local_time = *std::localtime(&now_time);

        // Print the local time in a human-readable format
        auditor().log_file << "@: " << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S")  // Custom format
                           << " = ";

        auditor().log_file << message << std::endl;
    }
};
