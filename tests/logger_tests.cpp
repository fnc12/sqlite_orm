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

#ifdef SQLITE_ORM_WITH_VIEW
struct UserViewLoggerTests {
    int id = 0;
    std::string name;
};
#endif
TEST_CASE("logger") {
    using Logs = std::vector<std::string>;
    using Callback = std::function<void(std::string_view)>;

    Logs expectedWillLogs;
    Logs expectedDidLogs;

    auto runRequire = [&expectedWillLogs, &expectedDidLogs] {
        REQUIRE(WillLogsCollector::logs == expectedWillLogs);
        REQUIRE(DidLogsCollector::logs == expectedDidLogs);
    };

    auto willRunQuery = GENERATE(Callback(WillLogsCollector()), Callback());
    auto didRunQuery = GENERATE(Callback(DidLogsCollector()), Callback());

    struct User {
        int64 id = 0;
        std::string name;
    };
    struct Visit {
        int64 id = 0;
        int64 userId = 0;
        std::string date;
    };
    struct VisitLog {
        int64 id = 0;
        std::string message;
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

    constexpr auto requireLogsAreEmpty = [] {
        REQUIRE(WillLogsCollector::logs.empty());
        REQUIRE(DidLogsCollector::logs.empty());
    };

    auto storage =
        make_storage("",
                     make_trigger("visits_log_insert",
                                  after()
                                      .insert()
                                      .on<Visit>()
                                      .for_each_row()
                                      .when(gt(new_(&Visit::userId), 0))
                                      .begin(insert(into<VisitLog>(),
                                                    columns(&VisitLog::message),
                                                    values(std::make_tuple("User " || c(new_(&Visit::userId)) ||
                                                                           " visited on " || new_(&Visit::date)))))
                                      .end()),
                     make_index("user_id_index", &User::id),
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
                     make_table("visits",
                                make_column("id", &Visit::id, primary_key()),
                                make_column("user_id", &Visit::userId),
                                make_column("date", &Visit::date)),
                     make_table("visits_log",
                                make_column("id", &VisitLog::id, primary_key()),
                                make_column("message", &VisitLog::message)),
#ifdef SQLITE_ORM_WITH_VIEW
                     make_view<UserViewLoggerTests>("users_view", select(asterisk<User>())),
#endif
                     will_run_query(willRunQuery),
                     did_run_query(didRunQuery));
    storage.sync_schema();

    WillLogsCollector::logs.clear();
    DidLogsCollector::logs.clear();

    SECTION("misc") {
        SECTION("transaction_guard") {
            enum class ActionAfter {
                rollback,
                commit,
                nothing,
            };

            auto guard = storage.transaction_guard();
            pushExpected("BEGIN TRANSACTION");
            runRequire();

            SECTION("nothing") {
                SECTION("nothing") {
                    pushExpected("ROLLBACK");
                }
                SECTION("commit_on_destroy = true") {
                    guard.commit_on_destroy = true;
                    pushExpected("COMMIT");
                }
                SECTION("commit_on_destroy = false") {
                    guard.commit_on_destroy = false;
                    pushExpected("ROLLBACK");
                }
            }
            SECTION("commit") {
                guard.commit();
                pushExpected("COMMIT");
            }
            SECTION("rollback") {
                guard.rollback();
                pushExpected("ROLLBACK");
            }
        }
        SECTION("drop_index") {
            storage.drop_index("user_id_index");
            pushExpected(R"(DROP INDEX "user_id_index")");
        }
        SECTION("drop_index_if_exists") {
            storage.drop_index_if_exists("user_id_index");
            pushExpected(R"(DROP INDEX IF EXISTS "user_id_index")");
        }
        SECTION("drop_trigger") {
            storage.drop_trigger("visits_log_insert");
            pushExpected(R"(DROP TRIGGER "visits_log_insert")");
        }
        SECTION("drop_trigger_if_exists") {
            const auto [value, expected] = GENERATE(table<std::string, std::string>({
                {"one", R"(DROP TRIGGER IF EXISTS "one")"},
                {"two", R"(DROP TRIGGER IF EXISTS "two")"},
            }));
            storage.drop_trigger_if_exists(value);
            pushExpected(expected);
        }
        SECTION("drop_table") {
            const auto [value, expected] = GENERATE(table<std::string, std::string>({
                {"users", R"(DROP TABLE "users")"},
                {"visits", R"(DROP TABLE "visits")"},
            }));
            storage.drop_table(value);
            pushExpected(expected);
        }
        SECTION("drop_table_if_exists") {
            const auto [value, expected] = GENERATE(table<std::string, std::string>({
                {"users", R"(DROP TABLE IF EXISTS "users")"},
                {"visits", R"(DROP TABLE IF EXISTS "visits")"},
            }));
            storage.drop_table_if_exists(value);
            pushExpected(expected);
        }
#ifdef SQLITE_ORM_WITH_VIEW
        SECTION("drop_view") {
            storage.drop_view("users_view");
            pushExpected(R"(DROP VIEW "users_view")");
        }
        SECTION("drop_view_if_exists") {
            const auto [value, expected] = GENERATE(table<std::string, std::string>({
                {"users_view", R"(DROP VIEW IF EXISTS "users_view")"},
                {"xyz_view", R"(DROP VIEW IF EXISTS "xyz_view")"},
            }));
            storage.drop_view_if_exists(value);
            pushExpected(expected);
        }
#endif
        SECTION("vacuum") {
            storage.vacuum();
            pushExpected("VACUUM");
        }
        SECTION("changes") {
            std::ignore = storage.changes();
        }
        SECTION("total_changes") {
            std::ignore = storage.total_changes();
        }
        SECTION("last_insert_rowid") {
            std::ignore = storage.last_insert_rowid();
        }
        SECTION("busy_timeout") {
            std::ignore = storage.busy_timeout(100);
        }
        SECTION("libversion") {
            std::ignore = storage.libversion();
        }
        SECTION("transaction") {
            using Transaction = std::function<bool()>;

            const auto [transactionLambda, expectedVector] = GENERATE(table<Transaction, std::vector<std::string>>({
                {Transaction(), {}},
                {Transaction([] {
                     return false;
                 }),
                 {"BEGIN TRANSACTION", "ROLLBACK"}},
                {Transaction([] {
                     return true;
                 }),
                 {"BEGIN TRANSACTION", "COMMIT"}},
            }));
            storage.transaction(transactionLambda);
            for (auto& expected: expectedVector) {
                pushExpected(expected);
            }
        }
        SECTION("current_time") {
            std::ignore = storage.current_time();
            pushExpected("SELECT CURRENT_TIME");
        }
        SECTION("current_date") {
            std::ignore = storage.current_date();
            pushExpected("SELECT CURRENT_DATE");
        }
        SECTION("current_timestamp") {
            std::ignore = storage.current_timestamp();
            pushExpected("SELECT CURRENT_TIMESTAMP");
        }
        SECTION("db_release_memory") {
            std::ignore = storage.db_release_memory();
        }
        SECTION("trigger_names") {
            std::ignore = storage.table_names();
            pushExpected("SELECT name FROM sqlite_master WHERE type='table'");
        }
        SECTION("view_names") {
            std::ignore = storage.view_names();
            pushExpected("SELECT name FROM sqlite_master WHERE type='view'");
        }
        SECTION("table_names") {
            std::ignore = storage.trigger_names();
            pushExpected("SELECT name FROM sqlite_master WHERE type='trigger'");
        }
        SECTION("open_forever") {
            storage.open_forever();
        }
        SECTION("begin_transaction") {
            using storage_base = internal::storage_base;

            enum class Action {
                commit,
                rollback,
                nothing,
            };

            const auto [transactionFunction, expected] = GENERATE(table<void (storage_base::*)(), std::string>({
                {&storage_base::begin_transaction, "BEGIN TRANSACTION"},
                {&storage_base::begin_deferred_transaction, "BEGIN DEFERRED TRANSACTION"},
                {&storage_base::begin_immediate_transaction, "BEGIN IMMEDIATE TRANSACTION"},
                {&storage_base::begin_exclusive_transaction, "BEGIN EXCLUSIVE TRANSACTION"},
            }));
            (storage.*transactionFunction)();
            pushExpected(expected);
            const auto postAction = GENERATE(Action::commit, Action::rollback, Action::nothing);
            switch (postAction) {
                case Action::commit:
                    storage.commit();
                    pushExpected("COMMIT");
                    break;
                case Action::rollback:
                    storage.rollback();
                    pushExpected("ROLLBACK");
                    break;
                case Action::nothing:
                    break;
            }
        }
        SECTION("filename") {
            std::ignore = storage.filename();
        }
        SECTION("is_opened") {
            std::ignore = storage.is_opened();
        }
        SECTION("get_autocommit") {
            std::ignore = storage.get_autocommit();
        }
        SECTION("busy_handler") {
            storage.busy_handler([](int argument) -> int {
                std::ignore = argument;
                return 0;
            });
        }
        SECTION("rename_table") {
            SECTION("in database") {
                storage.rename_table("users", "winners");
                pushExpected(R"(ALTER TABLE "users" RENAME TO "winners")");
            }
            SECTION("in storage schema") {
                storage.rename_table<User>("winners");
            }
        }
        SECTION("dump") {
            SECTION("users") {
                std::ignore = storage.dump(User{1, "Tate McRae"});
            }
            SECTION("visits") {
                std::ignore = storage.dump(Visit{1, 1, "today"});
            }
        }
        SECTION("tablename") {
            SECTION("users") {
                std::ignore = storage.tablename<User>();
            }
            SECTION("visits") {
                std::ignore = storage.tablename<Visit>();
            }
        }
        SECTION("find_column_name") {
            SECTION("users.id") {
                std::ignore = storage.find_column_name(&User::id);
            }
            SECTION("users.name") {
                std::ignore = storage.find_column_name(&User::name);
            }
            SECTION("visits.id") {
                std::ignore = storage.find_column_name(&Visit::id);
            }
            SECTION("visits.user_id") {
                std::ignore = storage.find_column_name(&Visit::userId);
            }
            SECTION("visits.date") {
                std::ignore = storage.find_column_name(&Visit::date);
            }
        }
        SECTION("table_exists") {
            SECTION("users") {
                std::ignore = storage.table_exists("users");
                pushExpected(R"(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'users')");
            }
            SECTION("visits") {
                std::ignore = storage.table_exists("visits");
                pushExpected(R"(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'visits')");
            }
            SECTION("non_existing") {
                std::ignore = storage.table_exists("non_existing");
                pushExpected(R"(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'non_existing')");
            }
        }
    }
    SECTION("functions") {
        SECTION("total") {
            SECTION("users") {
                SECTION("no conditions") {
                    std::ignore = storage.total(&User::id);
                    pushExpected(R"(SELECT TOTAL("users"."id") FROM "users")");
                }
                SECTION("where length(name) < 10") {
                    std::ignore = storage.total(&User::id, where(length(&User::name) < 10));
                    pushExpected(R"(SELECT TOTAL("users"."id") FROM "users" WHERE (LENGTH("users"."name") < ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    std::ignore = storage.total(&Visit::id);
                    pushExpected(R"(SELECT TOTAL("visits"."id") FROM "visits")");
                }
                SECTION("where user_id < 10") {
                    std::ignore = storage.total(&Visit::id, where(c(&Visit::userId) < 10));
                    pushExpected(R"(SELECT TOTAL("visits"."id") FROM "visits" WHERE ("visits"."user_id" < ?))");
                }
            }
        }
        SECTION("sum") {
            SECTION("users") {
                SECTION("no conditions") {
                    std::ignore = storage.sum(&User::id);
                    pushExpected(R"(SELECT SUM("users"."id") FROM "users")");
                }
                SECTION("where length(name) < 10") {
                    std::ignore = storage.sum(&User::id, where(length(&User::name) < 10));
                    pushExpected(R"(SELECT SUM("users"."id") FROM "users" WHERE (LENGTH("users"."name") < ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    std::ignore = storage.sum(&Visit::id);
                    pushExpected(R"(SELECT SUM("visits"."id") FROM "visits")");
                }
                SECTION("where user_id < 10") {
                    std::ignore = storage.sum(&Visit::id, where(c(&Visit::userId) < 10));
                    pushExpected(R"(SELECT SUM("visits"."id") FROM "visits" WHERE ("visits"."user_id" < ?))");
                }
            }
        }
        SECTION("min") {
            SECTION("users") {
                SECTION("no conditions") {
                    std::ignore = storage.min(&User::id);
                    pushExpected(R"(SELECT MIN("users"."id") FROM "users")");
                }
                SECTION("where length(name) < 10") {
                    std::ignore = storage.min(&User::id, where(length(&User::name) < 10));
                    pushExpected(R"(SELECT MIN("users"."id") FROM "users" WHERE (LENGTH("users"."name") < ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    std::ignore = storage.min(&Visit::id);
                    pushExpected(R"(SELECT MIN("visits"."id") FROM "visits")");
                }
                SECTION("where user_id < 10") {
                    std::ignore = storage.min(&Visit::id, where(c(&Visit::userId) < 10));
                    pushExpected(R"(SELECT MIN("visits"."id") FROM "visits" WHERE ("visits"."user_id" < ?))");
                }
            }
        }
        SECTION("max") {
            SECTION("users") {
                SECTION("no conditions") {
                    std::ignore = storage.max(&User::id);
                    pushExpected(R"(SELECT MAX("users"."id") FROM "users")");
                }
                SECTION("where length(name) < 10") {
                    std::ignore = storage.max(&User::id, where(length(&User::name) < 10));
                    pushExpected(R"(SELECT MAX("users"."id") FROM "users" WHERE (LENGTH("users"."name") < ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    std::ignore = storage.max(&Visit::id);
                    pushExpected(R"(SELECT MAX("visits"."id") FROM "visits")");
                }
                SECTION("where user_id < 10") {
                    std::ignore = storage.max(&Visit::id, where(c(&Visit::userId) < 10));
                    pushExpected(R"(SELECT MAX("visits"."id") FROM "visits" WHERE ("visits"."user_id" < ?))");
                }
            }
        }
        SECTION("group_concat") {
            SECTION("users") {
                SECTION("2 arguments") {
                    SECTION("no conditions") {
                        std::ignore = storage.group_concat(&User::name);
                        pushExpected(R"(SELECT GROUP_CONCAT("users"."name") FROM "users")");
                    }
                    SECTION("where id > 20") {
                        std::ignore = storage.group_concat(&User::name, where(c(&User::id) > 20));
                        pushExpected(R"(SELECT GROUP_CONCAT("users"."name") FROM "users" WHERE ("users"."id" > ?))");
                    }
                }
                SECTION("3 arguments") {
                    SECTION("no conditions") {
                        std::ignore = storage.group_concat(&User::name, "-");
                        pushExpected(R"(SELECT GROUP_CONCAT("users"."name", ?) FROM "users")");
                    }
                    SECTION("where id > 20") {
                        std::ignore = storage.group_concat(&User::name, "-", where(c(&User::id) > 20));
                        pushExpected(R"(SELECT GROUP_CONCAT("users"."name", ?) FROM "users" WHERE ("users"."id" > ?))");
                    }
                }
            }
            SECTION("visits") {
                SECTION("2 arguments") {
                    SECTION("no conditions") {
                        std::ignore = storage.group_concat(&Visit::date);
                        pushExpected(R"(SELECT GROUP_CONCAT("visits"."date") FROM "visits")");
                    }
                    SECTION("where id > 20") {
                        std::ignore = storage.group_concat(&Visit::date, where(c(&Visit::id) > 20));
                        pushExpected(R"(SELECT GROUP_CONCAT("visits"."date") FROM "visits" WHERE ("visits"."id" > ?))");
                    }
                }
                SECTION("3 arguments") {
                    SECTION("no conditions") {
                        std::ignore = storage.group_concat(&Visit::date, "-");
                        pushExpected(R"(SELECT GROUP_CONCAT("visits"."date", ?) FROM "visits")");
                    }
                    SECTION("where id > 20") {
                        std::ignore = storage.group_concat(&Visit::date, "-", where(c(&Visit::id) > 20));
                        pushExpected(
                            R"(SELECT GROUP_CONCAT("visits"."date", ?) FROM "visits" WHERE ("visits"."id" > ?))");
                    }
                }
            }
        }
        SECTION("avg") {
            SECTION("users") {
                SECTION("no conditions") {
                    std::ignore = storage.avg(&User::id);
                    pushExpected(R"(SELECT AVG("users"."id") FROM "users")");
                }
                SECTION("where id > 10") {
                    std::ignore = storage.avg(&User::id, where(c(&User::id) > 10));
                    pushExpected(R"(SELECT AVG("users"."id") FROM "users" WHERE ("users"."id" > ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    std::ignore = storage.avg(&Visit::id);
                    pushExpected(R"(SELECT AVG("visits"."id") FROM "visits")");
                }
                SECTION("where id > 10") {
                    std::ignore = storage.avg(&Visit::id, where(c(&Visit::id) > 10));
                    pushExpected(R"(SELECT AVG("visits"."id") FROM "visits" WHERE ("visits"."id" > ?))");
                }
            }
        }
        SECTION("count") {
            SECTION("count(*)") {
                SECTION("users") {
                    SECTION("no conditions") {
                        std::ignore = storage.count<User>();
                        pushExpected(R"(SELECT COUNT(*) FROM "users")");
                    }
                    SECTION("where id < 10") {
                        std::ignore = storage.count<User>(where(c(&User::id) < 10));
                        pushExpected(R"(SELECT COUNT(*) FROM "users" WHERE ("users"."id" < ?))");
                    }
                }
                SECTION("visits") {
                    SECTION("no conditions") {
                        std::ignore = storage.count<Visit>();
                        pushExpected(R"(SELECT COUNT(*) FROM "visits")");
                    }
                    SECTION("where id < 10") {
                        std::ignore = storage.count<Visit>(where(c(&Visit::id) < 10));
                        pushExpected(R"(SELECT COUNT(*) FROM "visits" WHERE ("visits"."id" < ?))");
                    }
                }
            }
            SECTION("count(X)") {
                SECTION("users") {
                    SECTION("no conditions") {
                        std::ignore = storage.count(&User::id);
                        pushExpected(R"(SELECT COUNT("users"."id") FROM "users")");
                    }
                    SECTION("where id < 20") {
                        std::ignore = storage.count(&User::id, where(c(&User::id) < 20));
                        pushExpected(R"(SELECT COUNT("users"."id") FROM "users" WHERE ("users"."id" < ?))");
                    }
                }
                SECTION("visits") {
                    SECTION("no conditions") {
                        std::ignore = storage.count(&Visit::id);
                        pushExpected(R"(SELECT COUNT("visits"."id") FROM "visits")");
                    }
                    SECTION("where id < 20") {
                        std::ignore = storage.count(&Visit::id, where(c(&Visit::id) < 20));
                        pushExpected(R"(SELECT COUNT("visits"."id") FROM "visits" WHERE ("visits"."id" < ?))");
                    }
                }
            }
        }
    }
    SECTION("non crud") {
        SECTION("insert_range") {
            SECTION("users") {
                const std::vector<User> usersToInsert = {
                    {1, "Tate McRae"},
                    {2, "Machine Gun Kelly"},
                };
                SECTION("simple") {
                    storage.insert_range(usersToInsert.begin(), usersToInsert.end());
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(insert_range(usersToInsert.begin(), usersToInsert.end()));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(INSERT INTO "users" ("name") VALUES (?), (?))");
            }
            SECTION("visits") {
                const std::vector<Visit> visitsToInsert = {
                    {1, 1, "today"},
                    {2, 10, "yesterday"},
                };
                SECTION("simple") {
                    storage.insert_range(visitsToInsert.begin(), visitsToInsert.end());
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(insert_range(visitsToInsert.begin(), visitsToInsert.end()));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(INSERT INTO "visits" ("user_id", "date") VALUES (?, ?), (?, ?))");
            }
        }
        SECTION("raw replace") {
            SECTION("users") {
                SECTION("name") {
                    SECTION("simple") {
                        storage.replace(into<User>(), columns(&User::name), values(std::make_tuple("TateMcRae")));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(
                            replace(into<User>(), columns(&User::name), values(std::make_tuple("TateMcRae"))));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(REPLACE INTO "users" ("name") VALUES (?))");
                }
                SECTION("id, name") {
                    SECTION("simple") {
                        storage.replace(into<User>(),
                                        columns(&User::id, &User::name),
                                        values(std::make_tuple(1, "TateMcRae")));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(replace(into<User>(),
                                                                 columns(&User::id, &User::name),
                                                                 values(std::make_tuple(1, "TateMcRae"))));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))");
                }
            }
            SECTION("visits") {
                SECTION("user_id, date") {
                    SECTION("simple") {
                        storage.replace(into<Visit>(),
                                        columns(&Visit::userId, &Visit::date),
                                        values(std::make_tuple(1, "today")));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(replace(into<Visit>(),
                                                                 columns(&Visit::userId, &Visit::date),
                                                                 values(std::make_tuple(1, "today"))));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(REPLACE INTO "visits" ("user_id", "date") VALUES (?, ?))");
                }
                SECTION("id, user_id, date") {
                    SECTION("simple") {
                        storage.replace(into<Visit>(),
                                        columns(&Visit::id, &Visit::userId, &Visit::date),
                                        values(std::make_tuple(1, 1, "today")));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(replace(into<Visit>(),
                                                                 columns(&Visit::id, &Visit::userId, &Visit::date),
                                                                 values(std::make_tuple(1, 1, "today"))));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(REPLACE INTO "visits" ("id", "user_id", "date") VALUES (?, ?, ?))");
                }
            }
        }
        SECTION("insert") {
            SECTION("implicit") {
                SECTION("users") {
                    SECTION("simple") {
                        storage.insert(User{1, "Tate McRae"});
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(insert(User{1, "Tate McRae"}));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(INSERT INTO "users" ("name") VALUES (?))");
                }
                SECTION("visits") {
                    SECTION("simple") {
                        storage.insert(Visit{1, 1, "today"});
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(insert(Visit{1, 1, "today"}));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(INSERT INTO "visits" ("user_id", "date") VALUES (?, ?))");
                }
            }
            SECTION("explicit + raw") {
                SECTION("users") {
                    SECTION("name") {
                        SECTION("explicit") {
                            SECTION("simple") {
                                storage.insert(User{1, "TateMcRae"}, columns(&User::name));
                            }
                            SECTION("prepared statement") {
                                auto statement = storage.prepare(insert(User{1, "TateMcRae"}, columns(&User::name)));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        SECTION("raw") {
                            SECTION("simple") {
                                storage.insert(into<User>(),
                                               columns(&User::name),
                                               values(std::make_tuple("TateMcRae")));
                            }
                            SECTION("prepared statement") {
                                auto statement = storage.prepare(
                                    insert(into<User>(), columns(&User::name), values(std::make_tuple("TateMcRae"))));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        pushExpected(R"(INSERT INTO "users" ("name") VALUES (?))");
                    }
                    SECTION("id, name") {
                        SECTION("explicit") {
                            SECTION("simple") {
                                storage.insert(User{1, "TateMcRae"}, columns(&User::id, &User::name));
                            }
                            SECTION("prepared statement") {
                                auto statement =
                                    storage.prepare(insert(User{1, "TateMcRae"}, columns(&User::id, &User::name)));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        SECTION("raw") {
                            SECTION("simple") {
                                storage.insert(into<User>(),
                                               columns(&User::id, &User::name),
                                               values(std::make_tuple(1, "TateMcRae")));
                            }
                            SECTION("prepared statement") {
                                auto statement = storage.prepare(insert(into<User>(),
                                                                        columns(&User::id, &User::name),
                                                                        values(std::make_tuple(1, "TateMcRae"))));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        pushExpected(R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))");
                    }
                }
                SECTION("visits") {
                    SECTION("user_id, date") {
                        SECTION("explicit") {
                            SECTION("simple") {
                                storage.insert(Visit{1, 1, "today"}, columns(&Visit::userId, &Visit::date));
                            }
                            SECTION("prepared statement") {
                                auto statement = storage.prepare(
                                    insert(Visit{1, 1, "today"}, columns(&Visit::userId, &Visit::date)));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        SECTION("raw") {
                            SECTION("simple") {
                                storage.insert(into<Visit>(),
                                               columns(&Visit::userId, &Visit::date),
                                               values(std::make_tuple(1, "today")));
                            }
                            SECTION("prepared statement") {
                                auto statement = storage.prepare(insert(into<Visit>(),
                                                                        columns(&Visit::userId, &Visit::date),
                                                                        values(std::make_tuple(1, "today"))));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        pushExpected(R"(INSERT INTO "visits" ("user_id", "date") VALUES (?, ?))");
                    }
                    SECTION("id, user_id, date") {
                        SECTION("explicit") {
                            SECTION("simple") {
                                storage.insert(Visit{1, 1, "today"}, columns(&Visit::id, &Visit::userId, &Visit::date));
                            }
                            SECTION("prepared statement") {
                                auto statement = storage.prepare(
                                    insert(Visit{1, 1, "today"}, columns(&Visit::id, &Visit::userId, &Visit::date)));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        SECTION("raw") {
                            SECTION("simple") {
                                storage.insert(into<Visit>(),
                                               columns(&Visit::id, &Visit::userId, &Visit::date),
                                               values(std::make_tuple(1, 1, "today")));
                            }
                            SECTION("prepared statement") {
                                auto statement =
                                    storage.prepare(insert(into<Visit>(),
                                                           columns(&Visit::id, &Visit::userId, &Visit::date),
                                                           values(std::make_tuple(1, 1, "today"))));
                                requireLogsAreEmpty();
                                storage.execute(statement);
                            }
                        }
                        pushExpected(R"(INSERT INTO "visits" ("id", "user_id", "date") VALUES (?, ?, ?))");
                    }
                }
            }
        }
        SECTION("select") {
            SECTION("users") {
                SECTION("simple") {
                    std::ignore = storage.select(&User::id);
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(select(&User::id));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(SELECT "users"."id" FROM "users")");
            }
            SECTION("visits") {
                SECTION("simple") {
                    std::ignore = storage.select(&Visit::id);
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(select(&Visit::id));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(SELECT "visits"."id" FROM "visits")");
            }
        }
        SECTION("get_all + get_all_pointer + get_all_optional") {
            SECTION("users") {
                SECTION("no conditions") {
                    SECTION("get_all") {
                        SECTION("simple") {
                            std::ignore = storage.get_all<User>();
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all<User>());
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
                    SECTION("get_all_pointer") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_pointer<User>();
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_pointer<User>());
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                    SECTION("get_all_optional") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_optional<User>();
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_optional<User>());
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
                    pushExpected(R"(SELECT "users"."id", "users"."name" FROM "users")");
                }
                SECTION("where id < 10") {
                    SECTION("get_all") {
                        SECTION("simple") {
                            std::ignore = storage.get_all<User>(where(c(&User::id) < 10));
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all<User>(where(c(&User::id) < 10)));
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
                    SECTION("get_all_pointer") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_pointer<User>(where(c(&User::id) < 10));
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_pointer<User>(where(c(&User::id) < 10)));
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                    SECTION("get_all_optional") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_optional<User>(where(c(&User::id) < 10));
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_optional<User>(where(c(&User::id) < 10)));
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
                    pushExpected(R"(SELECT "users"."id", "users"."name" FROM "users" WHERE ("users"."id" < ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    SECTION("get_all") {
                        SECTION("simple") {
                            std::ignore = storage.get_all<Visit>();
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all<Visit>());
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
                    SECTION("get_all_pointer") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_pointer<Visit>();
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_pointer<Visit>());
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                    SECTION("get_all_optional") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_optional<Visit>();
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_optional<Visit>());
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
                    pushExpected(R"(SELECT "visits"."id", "visits"."user_id", "visits"."date" FROM "visits")");
                }
                SECTION("where id < 10") {
                    SECTION("get_all") {
                        SECTION("simple") {
                            std::ignore = storage.get_all<Visit>(where(c(&Visit::id) < 10));
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all<Visit>(where(c(&Visit::id) < 10)));
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
                    SECTION("get_all_pointer") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_pointer<Visit>(where(c(&Visit::id) < 10));
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_pointer<Visit>(where(c(&Visit::id) < 10)));
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                    SECTION("get_all_optional") {
                        SECTION("simple") {
                            std::ignore = storage.get_all_optional<Visit>(where(c(&Visit::id) < 10));
                        }
                        SECTION("prepared statement") {
                            auto statement = storage.prepare(get_all_optional<Visit>(where(c(&Visit::id) < 10)));
                            requireLogsAreEmpty();
                            storage.execute(statement);
                        }
                    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
                    pushExpected(
                        R"(SELECT "visits"."id", "visits"."user_id", "visits"."date" FROM "visits" WHERE ("visits"."id" < ?))");
                }
            }
        }
        SECTION("remove_all") {
            SECTION("users") {
                SECTION("no conditions") {
                    SECTION("simple") {
                        storage.remove_all<User>();
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(remove_all<User>());
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(DELETE FROM "users")");
                }
                SECTION("where id = 1") {
                    SECTION("simple") {
                        storage.remove_all<User>(where(c(&User::id) == 1));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(remove_all<User>(where(c(&User::id) == 1)));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(DELETE FROM "users" WHERE ("users"."id" = ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    SECTION("simple") {
                        storage.remove_all<Visit>();
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(remove_all<Visit>());
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(DELETE FROM "visits")");
                }
                SECTION("where id = 1") {
                    SECTION("simple") {
                        storage.remove_all<Visit>(where(c(&Visit::id) == 1));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(remove_all<Visit>(where(c(&Visit::id) == 1)));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(DELETE FROM "visits" WHERE ("visits"."id" = ?))");
                }
            }
        }
        SECTION("update_all") {
            SECTION("users") {
                SECTION("no conditions") {
                    SECTION("simple") {
                        storage.update_all(set(c(&User::name) = "Jade"));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(update_all(set(c(&User::name) = "Jade")));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(UPDATE "users" SET "name" = ?)");
                }
                SECTION("where id = 10") {
                    SECTION("simple") {
                        storage.update_all(set(c(&User::name) = "Jade"), where(c(&User::id) == 10));
                    }
                    SECTION("prepared statement") {
                        auto statement =
                            storage.prepare(update_all(set(c(&User::name) = "Jade"), where(c(&User::id) == 10)));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(UPDATE "users" SET "name" = ? WHERE ("users"."id" = ?))");
                }
            }
            SECTION("visits") {
                SECTION("no conditions") {
                    SECTION("simple") {
                        storage.update_all(set(c(&Visit::date) = "Jade"));
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(update_all(set(c(&Visit::date) = "Jade")));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(UPDATE "visits" SET "date" = ?)");
                }
                SECTION("where id = 10") {
                    SECTION("simple") {
                        storage.update_all(set(c(&Visit::date) = "Jade"), where(c(&Visit::id) == 10));
                    }
                    SECTION("prepared statement") {
                        auto statement =
                            storage.prepare(update_all(set(c(&Visit::date) = "Jade"), where(c(&Visit::id) == 10)));
                        requireLogsAreEmpty();
                        storage.execute(statement);
                    }
                    pushExpected(R"(UPDATE "visits" SET "date" = ? WHERE ("visits"."id" = ?))");
                }
            }
        }
    }
    SECTION("crud") {
        SECTION("replace_range") {
            SECTION("users") {
                const std::vector<User> usersToReplace = {
                    {1, "Tate McRae"},
                    {2, "Machine Gun Kelly"},
                };
                SECTION("simple") {
                    storage.replace_range(usersToReplace.begin(), usersToReplace.end());
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(replace_range(usersToReplace.begin(), usersToReplace.end()));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?), (?, ?))");
            }
            SECTION("visits") {
                const std::vector<Visit> visitsToReplace = {
                    {1, 1, "today"},
                    {2, 10, "yesterday"},
                };
                SECTION("simple") {
                    storage.replace_range(visitsToReplace.begin(), visitsToReplace.end());
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(replace_range(visitsToReplace.begin(), visitsToReplace.end()));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(REPLACE INTO "visits" ("id", "user_id", "date") VALUES (?, ?, ?), (?, ?, ?))");
            }
        }
        SECTION("replace") {
            SECTION("users") {
                SECTION("simple") {
                    storage.replace(User{1, "Tate McRae"});
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(replace(User{1, "Tate McRae"}));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))");
            }
            SECTION("visits") {
                SECTION("simple") {
                    storage.replace(Visit{1, 5, "tomorrow"});
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(replace(Visit{1, 5, "tomorrow"}));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(REPLACE INTO "visits" ("id", "user_id", "date") VALUES (?, ?, ?))");
            }
        }
        SECTION("get + get_pointer + get_no_throw + get_optional") {
            SECTION("users") {
                storage.insert(User{1, "Tate McRae"});

                WillLogsCollector::logs.clear();
                DidLogsCollector::logs.clear();

                SECTION("get") {
                    SECTION("simple") {
                        std::ignore = storage.get<User>(1);
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(get<User>(1));
                        requireLogsAreEmpty();
                        std::ignore = storage.execute(statement);
                    }
                }
                SECTION("get_pointer") {
                    SECTION("simple") {
                        std::ignore = storage.get_pointer<User>(1);
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(get_pointer<User>(1));
                        requireLogsAreEmpty();
                        std::ignore = storage.execute(statement);
                    }
                }
                SECTION("get_no_throw") {
                    std::ignore = storage.get_no_throw<User>(1);
                }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                SECTION("get_optional") {
                    SECTION("simple") {
                        std::ignore = storage.get_optional<User>(1);
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(get_optional<User>(1));
                        requireLogsAreEmpty();
                        std::ignore = storage.execute(statement);
                    }
                }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
                pushExpected(R"(SELECT "id", "name" FROM "users" WHERE "id" = ?)");
            }
            SECTION("visits") {
                storage.insert(Visit{1, 1, "today"});

                WillLogsCollector::logs.clear();
                DidLogsCollector::logs.clear();

                SECTION("get") {
                    SECTION("simple") {
                        std::ignore = storage.get<Visit>(1);
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(get<Visit>(1));
                        requireLogsAreEmpty();
                        std::ignore = storage.execute(statement);
                    }
                }
                SECTION("get_pointer") {
                    SECTION("simple") {
                        std::ignore = storage.get_pointer<Visit>(1);
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(get_pointer<Visit>(1));
                        requireLogsAreEmpty();
                        std::ignore = storage.execute(statement);
                    }
                }
                SECTION("get_no_throw") {
                    std::ignore = storage.get_no_throw<Visit>(1);
                }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                SECTION("get_optional") {
                    SECTION("simple") {
                        std::ignore = storage.get_optional<Visit>(1);
                    }
                    SECTION("prepared statement") {
                        auto statement = storage.prepare(get_optional<Visit>(1));
                        requireLogsAreEmpty();
                        std::ignore = storage.execute(statement);
                    }
                }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
                pushExpected(R"(SELECT "id", "user_id", "date" FROM "visits" WHERE "id" = ?)");
            }
        }
        SECTION("remove") {
            SECTION("users") {
                SECTION("simple") {
                    storage.remove<User>(2);
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(remove<User>(2));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(DELETE FROM "users" WHERE "id" = ?)");
            }
            SECTION("visits") {
                SECTION("simple") {
                    storage.remove<Visit>(2);
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(remove<Visit>(2));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(DELETE FROM "visits" WHERE "id" = ?)");
            }
        }
        SECTION("update") {
            SECTION("users") {
                SECTION("simple") {
                    storage.update(User{1, "Tate McRae"});
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(update(User{1, "Tate McRae"}));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(UPDATE "users" SET "name" = ? WHERE "id" = ?)");
            }
            SECTION("visits") {
                SECTION("simple") {
                    storage.update(Visit{1, 2, "yesterday"});
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(update(Visit{1, 2, "yesterday"}));
                    requireLogsAreEmpty();
                    storage.execute(statement);
                }
                pushExpected(R"(UPDATE "visits" SET "user_id" = ?, "date" = ? WHERE "id" = ?)");
            }
        }
    }
    SECTION("pragma") {
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
    }
    runRequire();
}
