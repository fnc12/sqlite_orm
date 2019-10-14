#include <iostream>
#include <string>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <iterator>
#include "database_inspector.h"
#include "database_converter.h"

template <class C>
void split(const std::string& str, C& cont,
              char delim = ' ')
{
    std::size_t current, previous = 0;
    current = str.find(delim);
    while (current != std::string::npos) {
        cont.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find(delim, previous);
    }
    cont.push_back(str.substr(previous, current - previous));
}

void usage(const std::string& binary) {
    std::cout << "\nUsage:\n" << binary << " [Options] SOURCE DEST\n"
              << "\nThis tool creates the sqlite_orm classes and initialization"
              << "code from a db schema.  The SOURCE contains the schema either as a db"
              << "or as a sql file that describes a db. The DEST is a folder to which the code is generated\n\n"
              << "Options:\n"
              << "-s,\t--sql\tThe source contains sql instead of a sqlite db. Defaults to db file.\n"
              << "-t,\t--tables\tA comma seperated list of the tables that need to be exported. Defaults to all\n"
              << "-T,\t--template-dir\tThe directory with the templates to use. Defaults to CWD."
              << std::endl;
}

int main(int argc, char *argv[])
{
    bool doPrintUsage = true;
    try {
        bool useSql = false;
        std::vector<std::string> args;
        std::vector<std::string> tables;
        std::string templateDir(".");
        if(argc < 3) {
            throw std::runtime_error("not enough arguments");
        } else if (argc > 3) {
            args = std::vector<std::string>(argv + 1, argv + argc - 2);
        }
        std::string dest(argv[argc-1]);
        std::string src(argv[argc-2]);
        for (auto it = args.begin(); it != args.end(); ++it) {
            if(*it == "-s" || *it == "--sql") {
                useSql = true;
            } else if(*it == "-t" || *it == "--tables") {
                ++it;
                if(it == args.end()) {
                    throw std::runtime_error("the tables option requires you to specify the tables -> option not found");
                }
                split(*it, tables, ',');
            } else if(*it == "-T" || *it == "--template-dir") {
                ++it;
                if(it == args.end()) {
                    throw std::runtime_error("the template-dir option requires you to specify the directory -> option not found");
                }
                templateDir = *it;
            } else {
                throw std::runtime_error("unknown option given " + *it);
            }
        }

        if(useSql) {
            throw std::runtime_error("converting from a sql file is not yet implemented");
        }

        doPrintUsage = false;

        sqlite3* dbHandle;
        if(SQLITE_OK != sqlite3_open_v2(src.c_str(), &dbHandle, SQLITE_OPEN_READONLY, nullptr)) {
            throw std::runtime_error("can not open database " + src + "\nError: " + sqlite3_errmsg(dbHandle));
        }
        DatabaseInspector::DbConnection db(dbHandle, [](sqlite3* handle) { sqlite3_close(handle); });
        DatabaseInspector di(db);
        auto tablesCfg = di.readTablesFromDb(tables);
        di.readTableDetailsFromDb(tablesCfg);
        DatabaseConverter dc(templateDir, dest);
        dc.writeAll(tablesCfg);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        if(doPrintUsage) {
            usage(argv[0]);
        }
        return 1;
    }
    return 0;
}
