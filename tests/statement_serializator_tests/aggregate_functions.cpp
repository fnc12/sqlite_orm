#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator aggregate functions") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    std::string value;
    decltype(value) expected;

    SECTION("avg") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = avg(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(AVG(\"id\"))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "AVG(\"id\")";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = avg(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(AVG(\"id\")) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "AVG(\"id\") FILTER (WHERE \"id\" < 10)";
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
                    expected = "(COUNT(\"id\"))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "COUNT(\"id\")";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = count(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(COUNT(\"id\")) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "COUNT(\"id\") FILTER (WHERE \"id\" < 10)";
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
                expected = "COUNT(*)";
            }
            SECTION("without filter") {
                auto expression = count<User>().filter(where(lesser_than(&User::id, 10)));
                value = serialize(expression, context);
                expected = "COUNT(*) FILTER (WHERE (\"id\" < 10))";
            }
        }
    }
    SECTION("group_concat(X)") {
        SECTION("simple") {
            SECTION("with filter") {
                auto expression = group_concat(&User::id);
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(GROUP_CONCAT(\"id\"))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "GROUP_CONCAT(\"id\")";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = group_concat(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(GROUP_CONCAT(\"id\")) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "GROUP_CONCAT(\"id\") FILTER (WHERE \"id\" < 10)";
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
                    expected = "(GROUP_CONCAT(\"id\", '-'))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "GROUP_CONCAT(\"id\", '-')";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = group_concat(&User::id, "-").filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(GROUP_CONCAT(\"id\", '-')) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "GROUP_CONCAT(\"id\", '-') FILTER (WHERE \"id\" < 10)";
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
                    expected = "(MAX(\"id\"))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "MAX(\"id\")";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = max(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(MAX(\"id\")) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "MAX(\"id\") FILTER (WHERE \"id\" < 10)";
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
                    expected = "(MIN(\"id\"))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "MIN(\"id\")";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = min(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(MIN(\"id\")) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "MIN(\"id\") FILTER (WHERE \"id\" < 10)";
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
                    expected = "(SUM(\"id\"))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "SUM(\"id\")";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = sum(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(SUM(\"id\")) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "SUM(\"id\") FILTER (WHERE \"id\" < 10)";
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
                    expected = "(TOTAL(\"id\"))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "TOTAL(\"id\")";
                }
                value = serialize(expression, context);
            }
            SECTION("without filter") {
                auto expression = total(&User::id).filter(where(lesser_than(&User::id, 10)));
                SECTION("use_parentheses") {
                    context.use_parentheses = true;
                    expected = "(TOTAL(\"id\")) FILTER (WHERE (\"id\" < 10))";
                }
                SECTION("!use_parentheses") {
                    context.use_parentheses = false;
                    expected = "TOTAL(\"id\") FILTER (WHERE \"id\" < 10)";
                }
                value = serialize(expression, context);
            }
        }
    }
    REQUIRE(value == expected);
}
