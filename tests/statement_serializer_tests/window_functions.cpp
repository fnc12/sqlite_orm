#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer window functions") {
    using internal::serialize;

    struct T1 {
        int a = 0;
        std::string b;
        std::string c;
    };
    auto table = make_table("t1", make_column("a", &T1::a), make_column("b", &T1::b), make_column("c", &T1::c));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    std::string value;
    decltype(value) expected;

    SECTION("row_number") {
        // SQL: ROW_NUMBER() OVER (ORDER BY b)
        auto expression = row_number().over(order_by(&T1::b));
        value = serialize(expression, context);
        expected = R"(ROW_NUMBER() OVER (ORDER BY "t1"."b"))";
    }
    SECTION("rank") {
        // SQL: rank() OVER (ORDER BY b)
        auto expression = rank().over(order_by(&T1::b));
        value = serialize(expression, context);
        expected = R"(rank() OVER (ORDER BY "t1"."b"))";
    }
    SECTION("dense_rank") {
        // SQL: DENSE_RANK() OVER (ORDER BY a)
        auto expression = dense_rank().over(order_by(&T1::a));
        value = serialize(expression, context);
        expected = R"(DENSE_RANK() OVER (ORDER BY "t1"."a"))";
    }
    SECTION("percent_rank") {
        // SQL: PERCENT_RANK() OVER (ORDER BY a)
        auto expression = percent_rank().over(order_by(&T1::a));
        value = serialize(expression, context);
        expected = R"(PERCENT_RANK() OVER (ORDER BY "t1"."a"))";
    }
    SECTION("cume_dist") {
        // SQL: CUME_DIST() OVER (ORDER BY a)
        auto expression = cume_dist().over(order_by(&T1::a));
        value = serialize(expression, context);
        expected = R"(CUME_DIST() OVER (ORDER BY "t1"."a"))";
    }
    SECTION("ntile") {
        // SQL: NTILE(2) OVER (ORDER BY a)
        auto expression = ntile(2).over(order_by(&T1::a));
        value = serialize(expression, context);
        expected = R"(NTILE(2) OVER (ORDER BY "t1"."a"))";
    }
    SECTION("lag") {
        SECTION("one arg") {
            // SQL: LAG(b) OVER (ORDER BY a)
            auto expression = lag(&T1::b).over(order_by(&T1::a));
            value = serialize(expression, context);
            expected = R"(LAG("b") OVER (ORDER BY "t1"."a"))";
        }
        SECTION("two args") {
            // SQL: LAG(b, 2) OVER (ORDER BY a)
            auto expression = lag(&T1::b, 2).over(order_by(&T1::a));
            value = serialize(expression, context);
            expected = R"(LAG("b", 2) OVER (ORDER BY "t1"."a"))";
        }
        SECTION("three args") {
            // SQL: LAG(b, 2, 'n/a') OVER (ORDER BY b)
            auto expression = lag(&T1::b, 2, std::string("n/a")).over(order_by(&T1::b));
            value = serialize(expression, context);
            expected = R"(LAG("b", 2, 'n/a') OVER (ORDER BY "t1"."b"))";
        }
    }
    SECTION("lead") {
        SECTION("one arg") {
            // SQL: LEAD(b) OVER (ORDER BY a)
            auto expression = lead(&T1::b).over(order_by(&T1::a));
            value = serialize(expression, context);
            expected = R"(LEAD("b") OVER (ORDER BY "t1"."a"))";
        }
        SECTION("two args") {
            // SQL: LEAD(b, 2) OVER (ORDER BY a)
            auto expression = lead(&T1::b, 2).over(order_by(&T1::a));
            value = serialize(expression, context);
            expected = R"(LEAD("b", 2) OVER (ORDER BY "t1"."a"))";
        }
        SECTION("three args") {
            // SQL: LEAD(b, 2, 'n/a') OVER (ORDER BY b)
            auto expression = lead(&T1::b, 2, std::string("n/a")).over(order_by(&T1::b));
            value = serialize(expression, context);
            expected = R"(LEAD("b", 2, 'n/a') OVER (ORDER BY "t1"."b"))";
        }
    }
    SECTION("first_value") {
        // SQL: FIRST_VALUE(b) OVER (ORDER BY b ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
        auto expression = first_value(&T1::b).over(order_by(&T1::b), rows(unbounded_preceding(), current_row()));
        value = serialize(expression, context);
        expected = R"(FIRST_VALUE("b") OVER (ORDER BY "t1"."b" ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW))";
    }
    SECTION("last_value") {
        // SQL: LAST_VALUE(b) OVER (ORDER BY b ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
        auto expression = last_value(&T1::b).over(order_by(&T1::b), rows(unbounded_preceding(), current_row()));
        value = serialize(expression, context);
        expected = R"(LAST_VALUE("b") OVER (ORDER BY "t1"."b" ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW))";
    }
    SECTION("nth_value") {
        // SQL: NTH_VALUE(b, 3) OVER (ORDER BY b)
        auto expression = nth_value(&T1::b, 3).over(order_by(&T1::b));
        value = serialize(expression, context);
        expected = R"(NTH_VALUE("b", 3) OVER (ORDER BY "t1"."b"))";
    }
    SECTION("partition_by") {
        // SQL: DENSE_RANK() OVER (PARTITION BY c ORDER BY a)
        auto expression = dense_rank().over(partition_by(&T1::c), order_by(&T1::a));
        value = serialize(expression, context);
        expected = R"(DENSE_RANK() OVER (PARTITION BY "c" ORDER BY "t1"."a"))";
    }
    SECTION("frame ROWS") {
        // SQL: group_concat(b, '.') OVER (ORDER BY a ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING)
        auto expression =
            group_concat(&T1::b, std::string(".")).over(order_by(&T1::a), rows(preceding(1), following(1)));
        value = serialize(expression, context);
        expected = R"(GROUP_CONCAT("b", '.') OVER (ORDER BY "t1"."a" ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING))";
    }
    SECTION("frame RANGE") {
        // SQL: group_concat(b, '.') OVER (PARTITION BY c ORDER BY a RANGE BETWEEN CURRENT ROW AND UNBOUNDED FOLLOWING)
        auto expression =
            group_concat(&T1::b, std::string("."))
                .over(partition_by(&T1::c), order_by(&T1::a), range(current_row(), unbounded_following()));
        value = serialize(expression, context);
        expected =
            R"(GROUP_CONCAT("b", '.') OVER (PARTITION BY "c" ORDER BY "t1"."a" RANGE BETWEEN CURRENT ROW AND UNBOUNDED FOLLOWING))";
    }
    SECTION("frame GROUPS") {
        // SQL: group_concat(b, '.') OVER (ORDER BY c GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
        auto expression =
            group_concat(&T1::b, std::string(".")).over(order_by(&T1::c), groups(unbounded_preceding(), current_row()));
        value = serialize(expression, context);
        expected =
            R"(GROUP_CONCAT("b", '.') OVER (ORDER BY "t1"."c" GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW))";
    }
    SECTION("EXCLUDE") {
        SECTION("EXCLUDE CURRENT ROW") {
            // SQL: ... GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW EXCLUDE CURRENT ROW
            auto expression =
                group_concat(&T1::b, std::string("."))
                    .over(order_by(&T1::c), groups(unbounded_preceding(), current_row()).exclude_current_row());
            value = serialize(expression, context);
            expected =
                R"(GROUP_CONCAT("b", '.') OVER (ORDER BY "t1"."c" GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW EXCLUDE CURRENT ROW))";
        }
        SECTION("EXCLUDE GROUP") {
            auto expression = group_concat(&T1::b, std::string("."))
                                  .over(order_by(&T1::c), groups(unbounded_preceding(), current_row()).exclude_group());
            value = serialize(expression, context);
            expected =
                R"(GROUP_CONCAT("b", '.') OVER (ORDER BY "t1"."c" GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW EXCLUDE GROUP))";
        }
        SECTION("EXCLUDE TIES") {
            auto expression = group_concat(&T1::b, std::string("."))
                                  .over(order_by(&T1::c), groups(unbounded_preceding(), current_row()).exclude_ties());
            value = serialize(expression, context);
            expected =
                R"(GROUP_CONCAT("b", '.') OVER (ORDER BY "t1"."c" GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW EXCLUDE TIES))";
        }
    }
    SECTION("FILTER + OVER") {
        // SQL: group_concat(b, '.') FILTER (WHERE c != 'two') OVER (ORDER BY a)
        auto expression = group_concat(&T1::b, std::string("."))
                              .filter(where(c(&T1::c) != std::string("two")))
                              .over(order_by(&T1::a));
        value = serialize(expression, context);
        expected = R"(GROUP_CONCAT("b", '.') FILTER (WHERE "c" != 'two') OVER (ORDER BY "t1"."a"))";
    }
    SECTION("aggregate with OVER") {
        SECTION("sum") {
            // SQL: SUM(a) OVER (ORDER BY a)
            auto expression = sum(&T1::a).over(order_by(&T1::a));
            value = serialize(expression, context);
            expected = R"(SUM("a") OVER (ORDER BY "t1"."a"))";
        }
        SECTION("count") {
            // SQL: COUNT(a) OVER (PARTITION BY c)
            auto expression = count(&T1::a).over(partition_by(&T1::c));
            value = serialize(expression, context);
            expected = R"(COUNT("a") OVER (PARTITION BY "c"))";
        }
        SECTION("avg") {
            // SQL: AVG(a) OVER (ORDER BY a)
            auto expression = avg(&T1::a).over(order_by(&T1::a));
            value = serialize(expression, context);
            expected = R"(AVG("a") OVER (ORDER BY "t1"."a"))";
        }
    }
    SECTION("named window") {
        // SQL: WINDOW win AS (ORDER BY b ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
        auto expression = window("win", order_by(&T1::b), rows(unbounded_preceding(), current_row()));
        value = serialize(expression, context);
        expected = R"(WINDOW win AS (ORDER BY "t1"."b" ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW))";
    }
    SECTION("OVER named window reference") {
        // SQL: ROW_NUMBER() OVER win
        auto expression = row_number().over(window_ref("win"));
        value = serialize(expression, context);
        expected = R"(ROW_NUMBER() OVER win)";
    }
    REQUIRE(value == expected);
}
