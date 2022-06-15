#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

template<class E1, class E2>
static void assert_same(const E1&, const E2&) {
    STATIC_REQUIRE(std::is_same<E1, E2>::value);
}

TEST_CASE("statement_serializer insert/replace") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;

#if !defined(SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED) || !defined(SQLITE_ORM_AGGREGATE_BASES_SUPPORTED)
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
    using db_objects_t = internal::db_objects_tuple<decltype(table), decltype(table2)>;
    auto dbObjects = db_objects_t{table, table2};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
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
        SECTION("range") {
            context.replace_bindable_with_question = false;

            std::vector<User> users(1);
            SECTION("objects") {
                auto expression = replace_range<User>(users.begin(), users.end());
                // deduced object type
                assert_same(replace_range(users.begin(), users.end()), expression);
                // deduced object type
                assert_same(replace_range(users.begin(), users.end(), polyfill::identity{}), expression);
                assert_same(replace_range<User>(users.begin(), users.end(), polyfill::identity{}), expression);
                value = serialize(expression, context);
                expected = R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))";
            }
            SECTION("indirected") {
                std::vector<std::unique_ptr<User>> userPtrs;
                userPtrs.push_back(std::make_unique<User>(users.front()));
                auto expression =
                    replace_range<User>(userPtrs.begin(), userPtrs.end(), &std::unique_ptr<User>::operator*);
                // deduced object type
                assert_same(replace_range(userPtrs.begin(), userPtrs.end(), &std::unique_ptr<User>::operator*),
                            expression);
                value = serialize(expression, context);
                expected = R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))";
            }
            SECTION("wrapper") {
                std::vector<std::reference_wrapper<User>> userRefs{std::ref(users.front())};
                SECTION("identity") {
                    auto expression = replace_range<User>(userRefs.begin(), userRefs.end());
                    assert_same(replace_range<User>(userRefs.begin(), userRefs.end(), polyfill::identity{}),
                                expression);
                    value = serialize(expression, context);
                    expected = R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))";
                }
                SECTION("projected") {
                    auto expression =
                        replace_range<User>(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get);
#ifdef _MSC_VER /* `&std::reference_wrapper<long>::get` is only invocable with Microsoft STL */
                    // deduced object type
                    assert_same(replace_range(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get),
                                expression);
#endif
                    value = serialize(expression, context);
                    expected = R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))";
                }
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
        SECTION("range") {
            context.replace_bindable_with_question = false;

            std::vector<User> users(1);
            SECTION("objects") {
                auto expression = insert_range<User>(users.begin(), users.end());
                // deduced object type
                assert_same(insert_range(users.begin(), users.end()), expression);
                // deduced object type
                assert_same(insert_range(users.begin(), users.end(), polyfill::identity{}), expression);
                assert_same(insert_range<User>(users.begin(), users.end(), polyfill::identity{}), expression);
                value = serialize(expression, context);
                expected = R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))";
            }
            SECTION("indirected") {
                std::vector<std::unique_ptr<User>> userPtrs;
                userPtrs.push_back(std::make_unique<User>(users.front()));
                auto expression =
                    insert_range<User>(userPtrs.begin(), userPtrs.end(), &std::unique_ptr<User>::operator*);
                // deduced object type
                assert_same(insert_range(userPtrs.begin(), userPtrs.end(), &std::unique_ptr<User>::operator*),
                            expression);
                value = serialize(expression, context);
                expected = R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))";
            }
            SECTION("wrapper") {
                std::vector<std::reference_wrapper<User>> userRefs{std::ref(users.front())};
                SECTION("identity") {
                    auto expression = insert_range<User>(userRefs.begin(), userRefs.end());
                    // deduced object type
                    assert_same(insert_range<User>(userRefs.begin(), userRefs.end(), polyfill::identity{}), expression);
                    value = serialize(expression, context);
                    expected = R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))";
                }
                SECTION("projected") {
                    auto expression =
                        insert_range<User>(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get);
#ifdef _MSC_VER /* `&std::reference_wrapper<long>::get` is only invocable with Microsoft STL */
                    // deduced object type
                    assert_same(insert_range(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get),
                                expression);
#endif
                    value = serialize(expression, context);
                    expected = R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))";
                }
            }
        }
    }
    REQUIRE(value == expected);
}
