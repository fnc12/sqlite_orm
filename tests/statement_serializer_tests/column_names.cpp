#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer column names") {
    SECTION("by member field pointer") {
        struct User {
            int id = 0;
            std::string name;
        };
        auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
        using db_objects_t = internal::db_objects_tuple<decltype(table)>;
        db_objects_t dbObjects{table};
        {
            using context_t = internal::serializer_context<db_objects_t>;
            context_t context{dbObjects};
            SECTION("id") {
                SECTION("skip table name") {
                    auto value = serialize(&User::id, context);
                    REQUIRE(value == R"("id")");
                }
                SECTION("don't skip table name") {
                    context.skip_table_name = false;
                    auto value = serialize(&User::id, context);
                    REQUIRE(value == R"("users"."id")");
                }
            }
            SECTION("name") {
                auto value = serialize(&User::name, context);
                REQUIRE(value == R"("name")");
            }
        }
    }
    SECTION("by getters and setters pointers") {
        struct User {

            int getId() const {
                return this->id;
            }

            void setId(int value) {
                this->id = value;
            }

            const std::string& getName() const {
                return this->name;
            }

            void setName(std::string value) {
                this->name = move(value);
            }

          private:
            int id = 0;
            std::string name;
        };
        SECTION("getters, setters") {
            auto table = make_table("users",
                                    make_column("id", &User::getId, &User::setId),
                                    make_column("name", &User::getName, &User::setName));
            using db_objects_t = internal::db_objects_tuple<decltype(table)>;
            db_objects_t dbObjects{table};
            using context_t = internal::serializer_context<db_objects_t>;
            context_t context{dbObjects};
            std::string value;
            decltype(value) expected;
            SECTION("id") {
                SECTION("getter") {
                    value = serialize(&User::getId, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setId, context);
                }
                expected = R"("id")";
            }
            SECTION("name") {
                SECTION("getter") {
                    value = serialize(&User::getName, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setName, context);
                }
                expected = R"("name")";
            }
            REQUIRE(value == expected);
        }
        SECTION("setters, getters") {  //  column names by setters and getters pointers (reverse order)
            auto table = make_table("users",
                                    make_column("id", &User::setId, &User::getId),
                                    make_column("name", &User::setName, &User::getName));
            using db_objects_t = internal::db_objects_tuple<decltype(table)>;
            db_objects_t dbObjects{table};
            using context_t = internal::serializer_context<db_objects_t>;
            context_t context{dbObjects};
            std::string value;
            decltype(value) expected;
            SECTION("id") {
                SECTION("getter") {
                    value = serialize(&User::getId, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setId, context);
                }
                expected = R"("id")";
            }
            SECTION("name") {
                SECTION("getter") {
                    value = serialize(&User::getName, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setName, context);
                }
                expected = R"("name")";
            }
            REQUIRE(value == expected);
        }
    }
    // note: here we test whether the serializer serializes the correct column
    //       even if the object in question isn't the first in the table definition
    SECTION("by explicit column pointer") {
        struct Object1 {
            int id = 0;
        };
        struct Object2 {
            int id = 0;
        };
        auto table1 = make_table("object1", make_column("id1", &Object1::id));
        auto table2 = make_table("object2", make_column("id2", &Object2::id));
        using db_objects_t = internal::db_objects_tuple<decltype(table1), decltype(table2)>;
        db_objects_t dbObjects{table1, table2};
        {
            using context_t = internal::serializer_context<db_objects_t>;
            context_t context{dbObjects};
            SECTION("name") {
                auto value = serialize(column<Object2>(&Object2::id), context);
                REQUIRE(value == R"("id2")");
            }
        }
    }
    SECTION("escaped identifiers") {
        struct Object1 {
            int id = 0;
        };
        struct Object2 {
            int id = 0;
        };
        struct colalias : alias_tag {
            static std::string get() {
                return R"(a"s)";
            }
        };
        auto table1 = make_table(R"(ob"ject1)", make_column(R"(i"d)", &Object1::id));
        auto table2 = make_table(R"(ob"ject2)", make_column(R"(i"d)", &Object2::id));
        using db_objects_t = internal::db_objects_tuple<decltype(table1), decltype(table2)>;
        db_objects_t dbObjects{table1, table2};
        {
            using context_t = internal::serializer_context<db_objects_t>;
            context_t context{dbObjects};

            using als_d = alias_d<Object2>;
            auto expression =
                select(columns(&Object1::id, as<colalias>(&Object1::id), alias_column<als_d>(&Object2::id)),
                       join<als_d>(using_(&Object1::id)),
                       multi_order_by(order_by(get<colalias>()), order_by(alias_column<als_d>(&Object2::id))));
            expression.highest_level = true;
            auto value = serialize(expression, context);
            REQUIRE(value == R"(SELECT "ob""ject1"."i""d", "ob""ject1"."i""d" AS "a""s", "d"."i""d" FROM "ob""ject1" )"
                             R"(JOIN "ob""ject2" "d" USING ("i""d") ORDER BY "a""s", "d"."i""d")");
        }
    }
}
