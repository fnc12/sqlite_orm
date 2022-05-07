#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer insert/replace") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id, std::string name) : id{id}, name{move(name)} {}
#endif
    };
    struct UserBackup {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        UserBackup() = default;
        UserBackup(int id, std::string name) : id{id}, name{move(name)} {}
#endif
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    auto table2 =
        make_table("users_backup", make_column("id", &UserBackup::id), make_column("name", &UserBackup::name));
    using storage_impl_t = internal::storage_impl<decltype(table), decltype(table2)>;
    auto storageImpl = storage_impl_t{table, table2};
    using context_t = internal::serializer_context<storage_impl_t>;
    context_t context{storageImpl};
    std::string value;
    decltype(value) expected;
    SECTION("replace") {
        SECTION("object") {
            User user{5, "Gambit"};
            auto statement = replace(user);
            SECTION("question marks") {
                context.replace_bindable_with_question = true;
                expected = R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))";
            }
            SECTION("no question marks") {
                context.replace_bindable_with_question = false;
                expected = R"(REPLACE INTO "users" ("id", "name") VALUES (5, 'Gambit'))";
            }
            value = serialize(statement, context);
        }
        SECTION("raw") {
            SECTION("values") {
                SECTION("1 row") {
                    auto statement = replace(into<User>(),
                                             columns(&User::id, &User::name),
                                             values(std::make_tuple(1, "The Weeknd")));
                    value = serialize(statement, context);
                    expected = R"(REPLACE INTO "users" ("id", "name") VALUES (1, 'The Weeknd'))";
                }
                SECTION("2 rows") {
                    auto statement =
                        replace(into<User>(),
                                columns(&User::id, &User::name),
                                values(std::make_tuple(1, "The Weeknd"), std::make_tuple(4, "Jonas Blue")));
                    value = serialize(statement, context);
                    expected = R"(REPLACE INTO "users" ("id", "name") VALUES (1, 'The Weeknd'), (4, 'Jonas Blue'))";
                }
            }
            SECTION("default values") {
                auto statement = replace(into<User>(), default_values());
                value = serialize(statement, context);
                expected = R"(REPLACE INTO "users" DEFAULT VALUES)";
            }
            SECTION("select") {
                auto statement = replace(into<User>(), select(columns(&UserBackup::id, &UserBackup::name)));
                value = serialize(statement, context);
                expected =
                    R"(REPLACE INTO "users" SELECT "users_backup"."id", "users_backup"."name" FROM "users_backup")";
            }
        }
    }
    SECTION("insert") {
        User user{5, "Gambit"};
        SECTION("crud") {
            auto statement = insert(user);
            SECTION("question marks") {
                context.replace_bindable_with_question = true;
                expected = R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))";
            }
            SECTION("no question marks") {
                context.replace_bindable_with_question = false;
                expected = R"(INSERT INTO "users" ("id", "name") VALUES (5, 'Gambit'))";
            }
            value = serialize(statement, context);
        }
        SECTION("explicit") {
            SECTION("one column") {
                auto statement = insert(user, columns(&User::id));
                SECTION("question marks") {
                    context.replace_bindable_with_question = true;
                    expected = R"(INSERT INTO "users" ("id") VALUES (?))";
                }
                SECTION("no question marks") {
                    context.replace_bindable_with_question = false;
                    expected = R"(INSERT INTO "users" ("id") VALUES (5))";
                }
                value = serialize(statement, context);
            }
            SECTION("two columns") {
                auto statement = insert(user, columns(&User::id, &User::name));
                SECTION("question marks") {
                    context.replace_bindable_with_question = true;
                    expected = R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))";
                }
                SECTION("no question marks") {
                    context.replace_bindable_with_question = false;
                    expected = R"(INSERT INTO "users" ("id", "name") VALUES (5, 'Gambit'))";
                }
                value = serialize(statement, context);
            }
        }
        SECTION("raw") {
            SECTION("values") {
                SECTION("1 row") {
                    SECTION("no constraint") {
                        auto statement = insert(into<User>(),
                                                columns(&User::id, &User::name),
                                                values(std::make_tuple(1, "The Weeknd")));
                        value = serialize(statement, context);
                        expected = R"(INSERT INTO "users" ("id", "name") VALUES (1, 'The Weeknd'))";
                    }
                    SECTION("or abort") {
                        auto statement = insert(or_abort(),
                                                into<User>(),
                                                columns(&User::id, &User::name),
                                                values(std::make_tuple(1, "The Weeknd")));
                        value = serialize(statement, context);
                        expected = R"(INSERT OR ABORT INTO "users" ("id", "name") VALUES (1, 'The Weeknd'))";
                    }
                    SECTION("or fail") {
                        auto statement = insert(or_fail(),
                                                into<User>(),
                                                columns(&User::id, &User::name),
                                                values(std::make_tuple(1, "The Weeknd")));
                        value = serialize(statement, context);
                        expected = R"(INSERT OR FAIL INTO "users" ("id", "name") VALUES (1, 'The Weeknd'))";
                    }
                    SECTION("or ignore") {
                        auto statement = insert(or_ignore(),
                                                into<User>(),
                                                columns(&User::id, &User::name),
                                                values(std::make_tuple(1, "The Weeknd")));
                        value = serialize(statement, context);
                        expected = R"(INSERT OR IGNORE INTO "users" ("id", "name") VALUES (1, 'The Weeknd'))";
                    }
                    SECTION("or replace") {
                        auto statement = insert(or_replace(),
                                                into<User>(),
                                                columns(&User::id, &User::name),
                                                values(std::make_tuple(1, "The Weeknd")));
                        value = serialize(statement, context);
                        expected = R"(INSERT OR REPLACE INTO "users" ("id", "name") VALUES (1, 'The Weeknd'))";
                    }
                    SECTION("or rollback") {
                        auto statement = insert(or_rollback(),
                                                into<User>(),
                                                columns(&User::id, &User::name),
                                                values(std::make_tuple(1, "The Weeknd")));
                        value = serialize(statement, context);
                        expected = R"(INSERT OR ROLLBACK INTO "users" ("id", "name") VALUES (1, 'The Weeknd'))";
                    }
                }
                SECTION("2 rows") {
                    SECTION("no constraint") {
                        auto statement =
                            insert(into<User>(),
                                   columns(&User::id, &User::name),
                                   values(std::make_tuple(1, "The Weeknd"), std::make_tuple(4, "Jonas Blue")));
                        value = serialize(statement, context);
                        expected = R"(INSERT INTO "users" ("id", "name") VALUES (1, 'The Weeknd'), (4, 'Jonas Blue'))";
                    }
                    SECTION("or abort") {
                        auto statement =
                            insert(or_abort(),
                                   into<User>(),
                                   columns(&User::id, &User::name),
                                   values(std::make_tuple(1, "The Weeknd"), std::make_tuple(4, "Jonas Blue")));
                        value = serialize(statement, context);
                        expected =
                            R"(INSERT OR ABORT INTO "users" ("id", "name") VALUES (1, 'The Weeknd'), (4, 'Jonas Blue'))";
                    }
                    SECTION("or fail") {
                        auto statement =
                            insert(or_fail(),
                                   into<User>(),
                                   columns(&User::id, &User::name),
                                   values(std::make_tuple(1, "The Weeknd"), std::make_tuple(4, "Jonas Blue")));
                        value = serialize(statement, context);
                        expected =
                            R"(INSERT OR FAIL INTO "users" ("id", "name") VALUES (1, 'The Weeknd'), (4, 'Jonas Blue'))";
                    }
                    SECTION("or ignore") {
                        auto statement =
                            insert(or_ignore(),
                                   into<User>(),
                                   columns(&User::id, &User::name),
                                   values(std::make_tuple(1, "The Weeknd"), std::make_tuple(4, "Jonas Blue")));
                        value = serialize(statement, context);
                        expected =
                            R"(INSERT OR IGNORE INTO "users" ("id", "name") VALUES (1, 'The Weeknd'), (4, 'Jonas Blue'))";
                    }
                    SECTION("or replace") {
                        auto statement =
                            insert(or_replace(),
                                   into<User>(),
                                   columns(&User::id, &User::name),
                                   values(std::make_tuple(1, "The Weeknd"), std::make_tuple(4, "Jonas Blue")));
                        value = serialize(statement, context);
                        expected =
                            R"(INSERT OR REPLACE INTO "users" ("id", "name") VALUES (1, 'The Weeknd'), (4, 'Jonas Blue'))";
                    }
                    SECTION("or rollback") {
                        auto statement =
                            insert(or_rollback(),
                                   into<User>(),
                                   columns(&User::id, &User::name),
                                   values(std::make_tuple(1, "The Weeknd"), std::make_tuple(4, "Jonas Blue")));
                        value = serialize(statement, context);
                        expected =
                            R"(INSERT OR ROLLBACK INTO "users" ("id", "name") VALUES (1, 'The Weeknd'), (4, 'Jonas Blue'))";
                    }
                }
            }
            SECTION("default values") {
                SECTION("no constraint") {
                    auto statement = insert(into<User>(), default_values());
                    value = serialize(statement, context);
                    expected = R"(INSERT INTO "users" DEFAULT VALUES)";
                }
                SECTION("or abort") {
                    auto statement = insert(or_abort(), into<User>(), default_values());
                    value = serialize(statement, context);
                    expected = R"(INSERT OR ABORT INTO "users" DEFAULT VALUES)";
                }
                SECTION("or fail") {
                    auto statement = insert(or_fail(), into<User>(), default_values());
                    value = serialize(statement, context);
                    expected = R"(INSERT OR FAIL INTO "users" DEFAULT VALUES)";
                }
                SECTION("or ignore") {
                    auto statement = insert(or_ignore(), into<User>(), default_values());
                    value = serialize(statement, context);
                    expected = R"(INSERT OR IGNORE INTO "users" DEFAULT VALUES)";
                }
                SECTION("or replace") {
                    auto statement = insert(or_replace(), into<User>(), default_values());
                    value = serialize(statement, context);
                    expected = R"(INSERT OR REPLACE INTO "users" DEFAULT VALUES)";
                }
                SECTION("or rollback") {
                    auto statement = insert(or_rollback(), into<User>(), default_values());
                    value = serialize(statement, context);
                    expected = R"(INSERT OR ROLLBACK INTO "users" DEFAULT VALUES)";
                }
            }
            SECTION("select") {
                SECTION("no constraint") {
                    auto statement = insert(into<User>(), select(columns(&UserBackup::id, &UserBackup::name)));
                    value = serialize(statement, context);
                    expected =
                        R"(INSERT INTO "users" SELECT "users_backup"."id", "users_backup"."name" FROM "users_backup")";
                }
                SECTION("or abort") {
                    auto statement =
                        insert(or_abort(), into<User>(), select(columns(&UserBackup::id, &UserBackup::name)));
                    value = serialize(statement, context);
                    expected =
                        R"(INSERT OR ABORT INTO "users" SELECT "users_backup"."id", "users_backup"."name" FROM "users_backup")";
                }
                SECTION("or fail") {
                    auto statement =
                        insert(or_fail(), into<User>(), select(columns(&UserBackup::id, &UserBackup::name)));
                    value = serialize(statement, context);
                    expected =
                        R"(INSERT OR FAIL INTO "users" SELECT "users_backup"."id", "users_backup"."name" FROM "users_backup")";
                }
                SECTION("or ignore") {
                    auto statement =
                        insert(or_ignore(), into<User>(), select(columns(&UserBackup::id, &UserBackup::name)));
                    value = serialize(statement, context);
                    expected =
                        R"(INSERT OR IGNORE INTO "users" SELECT "users_backup"."id", "users_backup"."name" FROM "users_backup")";
                }
                SECTION("or replace") {
                    auto statement =
                        insert(or_replace(), into<User>(), select(columns(&UserBackup::id, &UserBackup::name)));
                    value = serialize(statement, context);
                    expected =
                        R"(INSERT OR REPLACE INTO "users" SELECT "users_backup"."id", "users_backup"."name" FROM "users_backup")";
                }
                SECTION("or rollback") {
                    auto statement =
                        insert(or_rollback(), into<User>(), select(columns(&UserBackup::id, &UserBackup::name)));
                    value = serialize(statement, context);
                    expected =
                        R"(INSERT OR ROLLBACK INTO "users" SELECT "users_backup"."id", "users_backup"."name" FROM "users_backup")";
                }
            }
        }
    }
    REQUIRE(value == expected);
}
