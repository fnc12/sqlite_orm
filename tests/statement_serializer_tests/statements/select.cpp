#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

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
                expected = "(SELECT null)";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT null";
            }
        }
    }
    SECTION("row") {
        auto statement = select(is_equal(std::make_tuple(1, 2, 3), std::make_tuple(4, 5, 6)));
        SECTION("!highest_level") {
            statement.highest_level = false;
            stringValue = serialize(statement, context);
            expected = "(SELECT ((1, 2, 3) = (4, 5, 6)))";
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            stringValue = serialize(statement, context);
            expected = "SELECT ((1, 2, 3) = (4, 5, 6))";
        }
    }
    SECTION("compound operator") {
        auto statement = select(union_(select(1), select(2)));
        stringValue = serialize(statement, context);
        expected = "SELECT 1 UNION SELECT 2";
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
                expected = "(SELECT null)";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT null";
            }
        }
        SECTION("asterisk") {
            SECTION("mapped") {
                auto expression = select(asterisk<User>());
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT * FROM "users")";
            }
            SECTION("mapped, implicit select order") {
                auto expression = select(asterisk<User>(false));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT * FROM "users")";
            }
            SECTION("mapped, defined select order") {
                auto expression = select(asterisk<User>(true));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "id", "name" FROM "users")";
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
                expected = R"(SELECT * FROM "users")";
            }
            SECTION("object, implicit select order") {
                auto expression = select(object<User>(false));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT * FROM "users")";
            }
            SECTION("object, defined select order") {
                auto expression = select(object<User>(true));
                expression.highest_level = true;
                stringValue = serialize(expression, context);
                expected = R"(SELECT "id", "name" FROM "users")";
            }
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
                                         left_join<als_e>(on(c(alias_column<als_d>(&Department::m_deptno)) ==
                                                             alias_column<als_e>(&Employee::m_deptno))),
                                         where(is_null(alias_column<als_e>(&Employee::m_deptno))));
                expression.highest_level = true;
                internal::serializer_context<db_objects_t> context{storage};
                context.skip_table_name = false;
                stringValue = serialize(expression, context);
                expected =
                    R"(SELECT "d".* FROM "Dept" "d" LEFT JOIN "Emp" "e" ON ("d"."deptno" = "e"."deptno")  WHERE ("e"."deptno" IS NULL))";
            }
        }
    }
    REQUIRE(stringValue == expected);
}
