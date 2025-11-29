#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

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
    };
    struct UserBackup {
        int id = 0;
        std::string name;
    };
    struct User2 {
        int id = 0;
        std::string name;
    };
    struct User3 {
        int id = 0;
        std::string name;
    };
    struct UserData1 {
        int userId = 0;
        int discriminatingId = 0;
    };
    struct UserData2 {
        int userId = 0;
    };
    struct UserData3 {
        int userId = 0;
    };

    // with rowid, no pk
    auto table1 = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    auto table2 =
        make_table("users_backup", make_column("id", &UserBackup::id), make_column("name", &UserBackup::name));
    // with rowid, column pk
    auto table3 = make_table("users2", make_column("id", &User2::id, primary_key()), make_column("name", &User2::name));
    auto table4 =
        make_table("users3", make_column("id", &User3::id), make_column("name", &User3::name), primary_key(&User3::id));
    // with rowid, composite table pk
    auto table5 = make_table("user_data1",
                             make_column("user_id", &UserData1::userId),
                             make_column("discriminating_id", &UserData1::discriminatingId),
                             primary_key(&UserData1::userId, &UserData1::discriminatingId));
    // without rowid, column pk
    auto table6 = make_table("user_data2", make_column("user_id", &UserData2::userId, primary_key())).without_rowid();
    // without rowid, table pk
    auto table7 = make_table("user_data3", make_column("user_id", &UserData3::userId), primary_key(&UserData3::userId))
                      .without_rowid();
    const std::tuple dbObjects = {table1, table2, table3, table4, table5, table6, table7};
    using db_objects_t = decltype(dbObjects);
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
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            SECTION("With clause") {
                constexpr orm_cte_moniker auto data = "data"_cte;
                constexpr auto cteExpression = cte<data>().as(select(asterisk<UserBackup>()));
                auto dbObjects2 =
                    internal::db_objects_cat(dbObjects, internal::make_cte_db_object(dbObjects, cteExpression));
                using context_t = internal::serializer_context<decltype(dbObjects2)>;
                context_t context2{dbObjects2};

                auto expression = with(cteExpression, replace(into<User>(), select(asterisk<data>())));
                value = serialize(expression, context2);
                expected =
                    R"(WITH "data"("id", "name") AS (SELECT "users_backup".* FROM "users_backup") REPLACE INTO "users" SELECT "data".* FROM "data")";
            }
#endif
#endif
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
#ifdef _MSC_VER /* `&std::reference_wrapper<long>::get` is only invocable with Microsoft STL and libstdc++ 15 */
                SECTION("projected") {
                    auto expression =
                        replace_range<User>(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get);
                    // deduced object type
                    assert_same(replace_range(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get),
                                expression);
                    value = serialize(expression, context);
                    expected = R"(REPLACE INTO "users" ("id", "name") VALUES (?, ?))";
                }
#endif
            }
        }
    }
    SECTION("insert") {
        User user{5, "Gambit"};
        User2 user2{5, "Gambit"};
        User3 user3{5, "Gambit"};
        UserData1 userData1{5, 5};
        UserData2 userData2{5};
        UserData3 userData3{5};
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
        SECTION("crud with rowid, column pk") {
            context.replace_bindable_with_question = false;
            auto statement = insert(user2);
            expected = R"(INSERT INTO "users2" ("name") VALUES ('Gambit'))";
            value = serialize(statement, context);
        }
        SECTION("crud with rowid, single table pk") {
            context.replace_bindable_with_question = false;
            auto statement = insert(user3);
            expected = R"(INSERT INTO "users3" ("name") VALUES ('Gambit'))";
            value = serialize(statement, context);
        }
        SECTION("crud with rowid, composite table pk") {
            context.replace_bindable_with_question = false;
            auto statement = insert(userData1);
            expected = R"(INSERT INTO "user_data1" ("user_id", "discriminating_id") VALUES (5, 5))";
            value = serialize(statement, context);
        }
        SECTION("crud without rowid, column pk") {
            context.replace_bindable_with_question = false;
            auto statement = insert(userData2);
            expected = R"(INSERT INTO "user_data2" ("user_id") VALUES (5))";
            value = serialize(statement, context);
        }
        SECTION("crud without rowid, table pk") {
            context.replace_bindable_with_question = false;
            auto statement = insert(userData3);
            expected = R"(INSERT INTO "user_data3" ("user_id") VALUES (5))";
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
            SECTION("With clause") {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
                constexpr orm_cte_moniker auto data = "data"_cte;
                constexpr auto cteExpression = cte<data>().as(select(asterisk<UserBackup>()));
                auto dbObjects2 =
                    internal::db_objects_cat(dbObjects, internal::make_cte_db_object(dbObjects, cteExpression));
                using context_t = internal::serializer_context<decltype(dbObjects2)>;
                context_t context2{dbObjects2};

                auto expression = with(cteExpression, insert(into<User>(), select(asterisk<data>())));
                value = serialize(expression, context2);
                expected =
                    R"(WITH "data"("id", "name") AS (SELECT "users_backup".* FROM "users_backup") INSERT INTO "users" SELECT "data".* FROM "data")";
#endif
#endif
            }
        }
        SECTION("range") {
            context.replace_bindable_with_question = false;

            std::vector<User> users(1);
            std::vector<User2> users2(1);
            std::vector<User3> users3(1);
            std::vector<UserData1> userData1(1);
            std::vector<UserData2> userData2(1);
            std::vector<UserData3> userData3(1);
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
#ifdef _MSC_VER /* `&std::reference_wrapper<long>::get` is only invocable with Microsoft STL and libstdc++ 15 */
                SECTION("projected") {
                    auto expression =
                        insert_range<User>(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get);
                    // deduced object type
                    assert_same(insert_range(userRefs.begin(), userRefs.end(), &std::reference_wrapper<User>::get),
                                expression);
                    value = serialize(expression, context);
                    expected = R"(INSERT INTO "users" ("id", "name") VALUES (?, ?))";
                }
#endif
            }
            SECTION("wit rowid, column pk") {
                auto expression = insert_range<User2>(users2.begin(), users2.end());
                value = serialize(expression, context);
                expected = R"(INSERT INTO "users2" ("name") VALUES (?))";
            }
            SECTION("with rowid, single table pk") {
                auto expression = insert_range<User3>(users3.begin(), users3.end());
                value = serialize(expression, context);
                expected = R"(INSERT INTO "users3" ("name") VALUES (?))";
            }
            SECTION("with rowid, composite table pk") {
                auto expression = insert_range<UserData1>(userData1.begin(), userData1.end());
                value = serialize(expression, context);
                expected = R"(INSERT INTO "user_data1" ("user_id", "discriminating_id") VALUES (?, ?))";
            }
            SECTION("without rowid, column pk") {
                auto expression = insert_range<UserData2>(userData2.begin(), userData2.end());
                value = serialize(expression, context);
                expected = R"(INSERT INTO "user_data2" ("user_id") VALUES (?))";
            }
            SECTION("without rowid, table pk") {
                auto expression = insert_range<UserData3>(userData3.begin(), userData3.end());
                value = serialize(expression, context);
                expected = R"(INSERT INTO "user_data3" ("user_id") VALUES (?))";
            }
        }
    }
    REQUIRE(value == expected);
}
