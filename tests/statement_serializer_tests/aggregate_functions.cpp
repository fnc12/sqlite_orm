#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer aggregate functions") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    std::string value;
    decltype(value) expected;

    SECTION("avg") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = avg(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((AVG("id")))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(AVG("id"))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = avg(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((AVG("id")) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(AVG("id") FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    SECTION("count(X)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = count(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((COUNT("id")))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(COUNT("id"))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = count(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((COUNT("id")) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(COUNT("id") FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    SECTION("count(*)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = count<User>();
                value = serialize(expression, context);
                expected = R"(COUNT(*))";
            }
            SECTION("without filter") {
                auto expression = count<User>().filter(where(lesser_than(&User::id, 10)));
                value = serialize(expression, context);
                expected = R"(COUNT(*) FILTER (WHERE ("id" < 10)))";
            }
        }
    }
    SECTION("group_concat(X)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = group_concat(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((GROUP_CONCAT("id")))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(GROUP_CONCAT("id"))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = group_concat(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((GROUP_CONCAT("id")) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(GROUP_CONCAT("id") FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    SECTION("group_concat(X,Y)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = group_concat(&User::id, "-");
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((GROUP_CONCAT("id", '-')))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(GROUP_CONCAT("id", '-'))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = group_concat(&User::id, "-").filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((GROUP_CONCAT("id", '-')) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(GROUP_CONCAT("id", '-') FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    SECTION("max(X)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = max(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((MAX("id")))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(MAX("id"))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = max(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((MAX("id")) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(MAX("id") FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    SECTION("min(X)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = min(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((MIN("id")))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(MIN("id"))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = min(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((MIN("id")) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(MIN("id") FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    SECTION("sum(X)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = sum(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((SUM("id")))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(SUM("id"))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = sum(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((SUM("id")) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(SUM("id") FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    SECTION("total(X)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = total(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((TOTAL("id")))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(TOTAL("id"))";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = total(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = R"((TOTAL("id")) FILTER (WHERE ("id" < 10)))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = R"(TOTAL("id") FILTER (WHERE "id" < 10))";
                }
                value = serialize(expression, context);
            }
        }
    }
    REQUIRE(value == expected);
}
