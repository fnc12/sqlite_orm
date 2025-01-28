#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer select_t") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    db_objects_t dbObjects{table};
    internal::serializer_context<db_objects_t> context{dbObjects};
    std::string stringValue;
    decltype(stringValue) expected;
    SECTION("simple") {
        SECTION("1") {
            auto statement = select(1);
            SECTION("!highest_level") {
                statement.highest_level = false;
                stringValue = serialize(statement, context);
                expected = "(SELECT 1)";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT 1";
            }
        }
        SECTION("null") {
            auto statement = select(nullptr);
            SECTION("!highest_level") {
                statement.highest_level = false;
                stringValue = serialize(statement, context);
                expected = "(SELECT NULL)";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT NULL";
            }
        }
    }
    SECTION("row") {
        auto statement = select(is_equal(std::make_tuple(1, 2, 3), std::make_tuple(4, 5, 6)));
        SECTION("!highest_level") {
            statement.highest_level = false;
            stringValue = serialize(statement, context);
            expected = "(SELECT (1, 2, 3) = (4, 5, 6))";
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            stringValue = serialize(statement, context);
            expected = "SELECT (1, 2, 3) = (4, 5, 6)";
        }
    }
    SECTION("compound operators") {
        SECTION("union") {
            auto statement = select(union_(select(1), select(2), select(3)));
            stringValue = serialize(statement, context);
            expected = "SELECT 1 UNION SELECT 2 UNION SELECT 3";
        }
        SECTION("union all") {
            auto statement = select(union_all(select(1), select(2), select(3)));
            stringValue = serialize(statement, context);
            expected = "SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3";
        }
        SECTION("except") {
            auto statement = select(except(select(1), select(2), select(3)));
            stringValue = serialize(statement, context);
            expected = "SELECT 1 EXCEPT SELECT 2 EXCEPT SELECT 3";
        }
        SECTION("intersect") {
            auto statement = select(intersect(select(1), select(2), select(3)));
            stringValue = serialize(statement, context);
            expected = "SELECT 1 INTERSECT SELECT 2 INTERSECT SELECT 3";
        }
    }
    SECTION("columns") {
        SECTION("literals") {
            auto statement = select(columns(1, 2));
            SECTION("!highest_level") {
                statement.highest_level = false;
                stringValue = serialize(statement, context);
                expected = "(SELECT 1, 2)";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT 1, 2";
            }
        }
        SECTION("from table") {
            auto statement = select(&User::id);
            SECTION("!highest_level") {
                statement.highest_level = false;
                stringValue = serialize(statement, context);
                expected = R"((SELECT "users"."id" FROM "users"))";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = R"(SELECT "users"."id" FROM "users")";
            }
        }
        SECTION("null") {
            auto statement = select(columns(nullptr));
            SECTION("!highest_level") {
                statement.highest_level = false;
                stringValue = serialize(statement, context);
                expected = "(SELECT NULL)";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT NULL";
            }
        }
        SECTION("asterisk") {
            SECTION("mapped") {
                auto expression = select(asterisk<User>());
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users".* FROM "users")";
            }
            SECTION("mapped, implicit select order") {
                auto expression = select(asterisk<User>(false));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users".* FROM "users")";
            }
            SECTION("mapped, defined select order") {
                auto expression = select(asterisk<User>(true));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users"."id", "users"."name" FROM "users")";
            }
            SECTION("alias, implicit select order") {
                using als_u = alias_u<User>;

                auto expression = select(asterisk<als_u>());
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "u".* FROM "users" "u")";
            }
            SECTION("alias, defined select order") {
                using als_u = alias_u<User>;

                auto expression = select(asterisk<als_u>(true));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "u"."id", "u"."name" FROM "users" "u")";
            }
            SECTION("object") {
                auto expression = select(object<User>());
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users".* FROM "users")";
            }
            SECTION("object, implicit select order") {
                auto expression = select(object<User>(false));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users".* FROM "users")";
            }
            SECTION("object, defined select order") {
                auto expression = select(object<User>(true));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users"."id", "users"."name" FROM "users")";
            }
            SECTION("multi object, defined select order") {
                auto expression = select(columns(object<User>(true), object<User>(true)));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users"."id", "users"."name", "users"."id", "users"."name" FROM "users")";
            }
            SECTION("struct") {
                auto expression = select(struct_<User>(asterisk<User>()));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users".* FROM "users")";
            }
            SECTION("multi struct") {
                auto expression = select(columns(struct_<User>(asterisk<User>()), struct_<User>(asterisk<User>())));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "users".*, "users".* FROM "users")";
            }
            // issue #1106
            SECTION("multi") {
                auto expression = columns(asterisk<User>(), asterisk<User>(true));
                context.skip_table_name = false;
                context.use_parentheses = false;
                stringValue = serialize(expression, context);
                expected = R"("users".*, "users"."id", "users"."name")";
            }
#if SQLITE_VERSION_NUMBER >= 3006019
            SECTION("issue #945") {
                struct Employee {
                    int m_empno;
                    int m_deptno;
                };

                struct Department {
                    int m_deptno;
                    std::string m_deptname;
                };

                auto t1 = make_table("Emp",
                                     make_column("empno", &Employee::m_empno, primary_key().autoincrement()),
                                     make_column("deptno", &Employee::m_deptno),
                                     foreign_key(&Employee::m_deptno).references(&Department::m_deptno));
                auto t2 =
                    make_table("Dept", make_column("deptno", &Department::m_deptno, primary_key().autoincrement()));

                using db_objects_t = internal::db_objects_tuple<decltype(t1), decltype(t2)>;
                db_objects_t storage{t1, t2};

                using als_d = alias_d<Department>;
                using als_e = alias_e<Employee>;

                auto expression = select(asterisk<als_d>(),
                                         left_join<als_e>(on(alias_column<als_d>(&Department::m_deptno) ==
                                                             alias_column<als_e>(&Employee::m_deptno))),
                                         where(is_null(alias_column<als_e>(&Employee::m_deptno))));
                expression.highest_level = true;
                internal::serializer_context<db_objects_t> context{storage};
                context.skip_table_name = false;
                stringValue = serialize(expression, context);
                expected =
                    R"(SELECT "d".* FROM "Dept" "d" LEFT JOIN "Emp" "e" ON "d"."deptno" = "e"."deptno"  WHERE ("e"."deptno" IS NULL))";
            }
#endif
        }
        SECTION("deduplication") {
            SECTION("distinct column, top-level") {
                auto expression = select(distinct(&User::name));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT DISTINCT "users"."name" FROM "users")";
            }
            SECTION("distinct column, !top-level") {
                auto expression = select(distinct(&User::name));
                expression.highest_level = false;
                stringValue = serialize(expression, context);
                expected = R"((SELECT DISTINCT "users"."name" FROM "users"))";
            }
            SECTION("all column") {
                auto expression = select(all(&User::name));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT ALL "users"."name" FROM "users")";
            }
            SECTION("distinct columns") {
                auto expression = select(distinct(columns(&User::id, &User::name)));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT DISTINCT "users"."id", "users"."name" FROM "users")";
            }
            SECTION("all columns") {
                auto expression = select(all(columns(&User::id, &User::name)));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT ALL "users"."id", "users"."name" FROM "users")";
            }
            SECTION("distinct struct") {
                auto expression = select(distinct(struct_<User>(&User::name)));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT DISTINCT "users"."name" FROM "users")";
            }
            SECTION("all struct") {
                auto expression = select(all(struct_<User>(&User::name)));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT ALL "users"."name" FROM "users")";
            }
            SECTION("distinct aggregate function, top-level") {
                auto expression = select(count(distinct(&User::name)));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT COUNT(DISTINCT "users"."name") FROM "users")";
            }
            SECTION("distinct aggregate function, !top-level") {
                auto expression = select(count(distinct(&User::name)));
                expression.highest_level = false;
                stringValue = serialize(expression, context);
                expected = R"((SELECT COUNT(DISTINCT "users"."name") FROM "users"))";
            }
        }
    }
    REQUIRE(stringValue == expected);
}
