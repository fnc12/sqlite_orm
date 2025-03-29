#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

struct WillLogsCollector {
    static std::vector<std::string> logs;

    void operator()(const std::string_view log) {
        this->logs.push_back(std::string(log));
    }
};

std::vector<std::string> WillLogsCollector::logs;

struct DidLogsCollector {
    static std::vector<std::string> logs;

    void operator()(const std::string_view log) {
        this->logs.push_back(std::string(log));
    }
};

std::vector<std::string> DidLogsCollector::logs;

TEST_CASE("logger") {
    using Logs = std::vector<std::string>;
    using Callback = std::function<void(std::string_view)>;

    Logs expectedWillLogs;
    Logs expectedDidLogs;

    auto willRunQuery = GENERATE(Callback(WillLogsCollector()), Callback());
    auto didRunQuery = GENERATE(Callback(DidLogsCollector()), Callback());

    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        int userId = 0;
        std::string date;
    };

    auto pushExpected =
        [&willRunQuery, &didRunQuery, &expectedWillLogs, &expectedDidLogs](const std::string& expected) {
            if (willRunQuery) {
                expectedWillLogs.push_back(expected);
            }
            if (didRunQuery) {
                expectedDidLogs.push_back(expected);
            }
        };

    auto storage =
        make_storage("",
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
                     make_table("visits",
                                make_column("id", &Visit::id, primary_key()),
                                make_column("user_id", &Visit::userId),
                                make_column("date", &Visit::date)),
                     will_run_query(std::move(willRunQuery)),
                     did_run_query(std::move(didRunQuery)));
    storage.sync_schema();

    WillLogsCollector::logs.clear();
    DidLogsCollector::logs.clear();

    SECTION("application_id") {
        SECTION("get") {
            std::ignore = storage.pragma.application_id();
            pushExpected("PRAGMA application_id");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<int, std::string>({{2, "PRAGMA application_id = 2"},
                                                                             {3, "PRAGMA application_id = 3"},
                                                                             {4, "PRAGMA application_id = 4"}}));
            storage.pragma.application_id(value);
            pushExpected(expected);
        }
    }
    SECTION("module_list") {
        std::ignore = storage.pragma.module_list();
        pushExpected("PRAGMA module_list");
    }
    SECTION("recursive_triggers") {
        SECTION("get") {
            std::ignore = storage.pragma.recursive_triggers();
            pushExpected("PRAGMA recursive_triggers");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<bool, std::string>({
                {true, "PRAGMA recursive_triggers = 1"},
                {false, "PRAGMA recursive_triggers = 0"},
            }));
            storage.pragma.recursive_triggers(value);
            pushExpected(expected);
        }
    }
    SECTION("busy_timeout") {
        SECTION("get") {
            std::ignore = storage.pragma.busy_timeout();
            pushExpected("PRAGMA busy_timeout");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<int, std::string>({
                {1, "PRAGMA busy_timeout = 1"},
                {2, "PRAGMA busy_timeout = 2"},
                {3, "PRAGMA busy_timeout = 3"},
            }));
            storage.pragma.busy_timeout(value);
            pushExpected(expected);
        }
    }
    SECTION("locking_mode") {
        SECTION("get") {
            std::ignore = storage.pragma.locking_mode();
            pushExpected("PRAGMA locking_mode");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<locking_mode, std::string>({
                {locking_mode::NORMAL, "PRAGMA locking_mode = NORMAL"},
                {locking_mode::EXCLUSIVE, "PRAGMA locking_mode = EXCLUSIVE"},
            }));
            storage.pragma.locking_mode(value);
            pushExpected(expected);
        }
    }
    SECTION("journal_mode") {
        SECTION("get") {
            std::ignore = storage.pragma.journal_mode();
            pushExpected("PRAGMA journal_mode");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<journal_mode, std::string>({
                {journal_mode::DELETE_, "PRAGMA journal_mode = DELETE"},
                {journal_mode::TRUNCATE, "PRAGMA journal_mode = TRUNCATE"},
                {journal_mode::PERSIST, "PRAGMA journal_mode = PERSIST"},
                {journal_mode::MEMORY, "PRAGMA journal_mode = MEMORY"},
                {journal_mode::WAL, "PRAGMA journal_mode = WAL"},
                {journal_mode::OFF, "PRAGMA journal_mode = OFF"},
            }));
            storage.pragma.journal_mode(value);
            pushExpected(expected);
        }
    }
    SECTION("synchronous") {
        SECTION("get") {
            std::ignore = storage.pragma.synchronous();
            pushExpected("PRAGMA synchronous");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<int, std::string>({
                {0, "PRAGMA synchronous = 0"},
                {1, "PRAGMA synchronous = 1"},
                {2, "PRAGMA synchronous = 2"},
                {3, "PRAGMA synchronous = 3"},
            }));
            storage.pragma.synchronous(value);
            pushExpected(expected);
        }
    }
    SECTION("user_version") {
        SECTION("get") {
            std::ignore = storage.pragma.user_version();
            pushExpected("PRAGMA user_version");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<int, std::string>({
                {0, "PRAGMA user_version = 0"},
                {1, "PRAGMA user_version = 1"},
                {2, "PRAGMA user_version = 2"},
                {3, "PRAGMA user_version = 3"},
                {4, "PRAGMA user_version = 4"},
            }));
            storage.pragma.user_version(value);
            pushExpected(expected);
        }
    }
    SECTION("auto_vacuum") {
        SECTION("get") {
            std::ignore = storage.pragma.auto_vacuum();
            pushExpected("PRAGMA auto_vacuum");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<int, std::string>({
                {0, "PRAGMA auto_vacuum = 0"},
                {1, "PRAGMA auto_vacuum = 1"},
                {2, "PRAGMA auto_vacuum = 2"},
            }));
            storage.pragma.auto_vacuum(value);
            pushExpected(expected);
        }
    }
    SECTION("max_page_count") {
        SECTION("get") {
            std::ignore = storage.pragma.max_page_count();
            pushExpected("PRAGMA max_page_count");
        }
        SECTION("set") {
            const auto [value, expected] = GENERATE(table<int, std::string>({
                {10, "PRAGMA max_page_count = 10"},
                {20, "PRAGMA max_page_count = 20"},
                {30, "PRAGMA max_page_count = 30"},
            }));
            storage.pragma.max_page_count(value);
            pushExpected(expected);
        }
    }
    SECTION("integrity_check") {
        SECTION("get") {
            std::ignore = storage.pragma.integrity_check();
            pushExpected("PRAGMA integrity_check");
        }
        SECTION("set table-name") {
            const auto [value, expected] = GENERATE(table<std::string, std::string>({
                {"users", "PRAGMA integrity_check(users)"},
                {"visits", "PRAGMA integrity_check(visits)"},
            }));
            storage.pragma.integrity_check(value);
            pushExpected(expected);
        }
        SECTION("set N") {
            const auto [value, expected] = GENERATE(table<int, std::string>({
                {1, "PRAGMA integrity_check(1)"},
                {2, "PRAGMA integrity_check(2)"},
            }));
            storage.pragma.integrity_check(value);
            pushExpected(expected);
        }
    }
    SECTION("quick_check") {
        std::ignore = storage.pragma.quick_check();
        pushExpected("PRAGMA quick_check");
    }
    SECTION("table_xinfo") {
        const auto [value, expected] = GENERATE(table<std::string, std::string>({
            {"users", R"(PRAGMA table_xinfo("users"))"},
            {"visits", R"(PRAGMA table_xinfo("visits"))"},
        }));
        std::ignore = storage.pragma.table_xinfo(value);
        pushExpected(expected);
    }
    SECTION("table_info") {
        const auto [value, expected] = GENERATE(table<std::string, std::string>({
            {"users", R"(PRAGMA table_info("users"))"},
            {"visits", R"(PRAGMA table_info("visits"))"},
        }));
        std::ignore = storage.pragma.table_info(value);
        pushExpected(expected);
    }
    REQUIRE(WillLogsCollector::logs == expectedWillLogs);
    REQUIRE(DidLogsCollector::logs == expectedDidLogs);
}
