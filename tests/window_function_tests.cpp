#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("window functions") {
    struct DenseRankDemo {
        int id = 0;
        std::string val;
    };

    struct T1 {
        int a = 0;
        std::string b;
        std::string c;
    };

    struct T2 {
        std::string a;
        std::string b;
    };

    auto storage = make_storage(
        "",
        make_table("DenseRankDemo",
                   make_column("id", &DenseRankDemo::id, primary_key().autoincrement()),
                   make_column("Val", &DenseRankDemo::val)),
        make_table("t1", make_column("a", &T1::a, primary_key()), make_column("b", &T1::b), make_column("c", &T1::c)),
        make_table("t2", make_column("a", &T2::a), make_column("b", &T2::b)));
    storage.sync_schema();

    SECTION("DENSE_RANK - Issue #1478") {
        // SQL: INSERT INTO DenseRankDemo(Val) VALUES('A'),('B'),('C'),('C'),('D'),('D'),('E');
        storage.insert(DenseRankDemo{0, "A"});
        storage.insert(DenseRankDemo{0, "B"});
        storage.insert(DenseRankDemo{0, "C"});
        storage.insert(DenseRankDemo{0, "C"});
        storage.insert(DenseRankDemo{0, "D"});
        storage.insert(DenseRankDemo{0, "D"});
        storage.insert(DenseRankDemo{0, "E"});

        // SQL: SELECT Val, DENSE_RANK() OVER (ORDER BY Val) ValRank FROM DenseRankDemo
        auto rows = storage.select(columns(&DenseRankDemo::val, dense_rank().over(order_by(&DenseRankDemo::val))));

        REQUIRE(rows.size() == 7);
        // A=1, B=2, C=3, C=3, D=4, D=4, E=5
        REQUIRE(std::get<1>(rows[0]) == 1);
        REQUIRE(std::get<1>(rows[1]) == 2);
        REQUIRE(std::get<1>(rows[2]) == 3);
        REQUIRE(std::get<1>(rows[3]) == 3);
        REQUIRE(std::get<1>(rows[4]) == 4);
        REQUIRE(std::get<1>(rows[5]) == 4);
        REQUIRE(std::get<1>(rows[6]) == 5);
    }
    SECTION("ROW_NUMBER with PARTITION BY") {
        storage.replace(T2{"a", "one"});
        storage.replace(T2{"a", "two"});
        storage.replace(T2{"a", "three"});
        storage.replace(T2{"b", "four"});
        storage.replace(T2{"c", "five"});
        storage.replace(T2{"c", "six"});

        // SQL: SELECT a, row_number() OVER (PARTITION BY a ORDER BY b) FROM t2
        auto rows = storage.select(columns(&T2::a, row_number().over(partition_by(&T2::a), order_by(&T2::b))));

        REQUIRE(rows.size() == 6);
        // Each partition restarts numbering
        int maxRowNum = 0;
        std::string lastA;
        for (auto& [a, rn]: rows) {
            if (a != lastA) {
                lastA = a;
                maxRowNum = 0;
            }
            REQUIRE(rn > maxRowNum);
            maxRowNum = rn;
        }
    }
    SECTION("Ranking functions") {
        storage.replace(T2{"a", "one"});
        storage.replace(T2{"a", "two"});
        storage.replace(T2{"a", "three"});
        storage.replace(T2{"b", "four"});
        storage.replace(T2{"c", "five"});
        storage.replace(T2{"c", "six"});

        // SQL: SELECT a, row_number() OVER win, rank() OVER win, dense_rank() OVER win,
        //             percent_rank() OVER win, cume_dist() OVER win
        //      FROM t2 WINDOW win AS (ORDER BY a)
        auto rows = storage.select(columns(&T2::a,
                                           row_number().over(order_by(&T2::a)),
                                           rank().over(order_by(&T2::a)),
                                           dense_rank().over(order_by(&T2::a)),
                                           percent_rank().over(order_by(&T2::a)),
                                           cume_dist().over(order_by(&T2::a))));

        REQUIRE(rows.size() == 6);
        // row 1: a='a', row_number=1, rank=1, dense_rank=1
        REQUIRE(std::get<1>(rows[0]) == 1);
        REQUIRE(std::get<2>(rows[0]) == 1);
        REQUIRE(std::get<3>(rows[0]) == 1);
    }
    SECTION("LAG and LEAD") {
        storage.replace(T1{1, "A", "one"});
        storage.replace(T1{2, "B", "two"});
        storage.replace(T1{3, "C", "three"});
        storage.replace(T1{4, "D", "one"});
        storage.replace(T1{5, "E", "two"});

        // SQL: SELECT b, lag(b) OVER (ORDER BY a), lead(b) OVER (ORDER BY a) FROM t1
        auto rows =
            storage.select(columns(&T1::b, lag(&T1::b).over(order_by(&T1::a)), lead(&T1::b).over(order_by(&T1::a))));

        REQUIRE(rows.size() == 5);
        // First row: lag is NULL (empty string for std::string), lead is "B"
        REQUIRE(std::get<1>(rows[0]) == "");
        REQUIRE(std::get<2>(rows[0]) == "B");
        // Last row: lag is "D", lead is NULL (empty string)
        REQUIRE(std::get<1>(rows[4]) == "D");
        REQUIRE(std::get<2>(rows[4]) == "");
    }
    SECTION("FIRST_VALUE, LAST_VALUE, NTH_VALUE") {
        storage.replace(T1{1, "A", "one"});
        storage.replace(T1{2, "B", "two"});
        storage.replace(T1{3, "C", "three"});
        storage.replace(T1{4, "D", "one"});
        storage.replace(T1{5, "E", "two"});

        // SQL: SELECT b, first_value(b) OVER win, last_value(b) OVER win, nth_value(b, 3) OVER win
        //      FROM t1 WINDOW win AS (ORDER BY b ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
        auto frameSpec = rows(unbounded_preceding(), current_row());
        auto result = storage.select(columns(&T1::b,
                                             first_value(&T1::b).over(order_by(&T1::b), frameSpec),
                                             last_value(&T1::b).over(order_by(&T1::b), frameSpec),
                                             nth_value(&T1::b, 3).over(order_by(&T1::b), frameSpec)));

        REQUIRE(result.size() == 5);
        for (auto& row: result) {
            REQUIRE(std::get<1>(row) == "A");
        }
        REQUIRE(std::get<2>(result[0]) == "A");
        REQUIRE(std::get<2>(result[1]) == "B");
    }
    SECTION("Aggregate with OVER and frame") {
        storage.replace(T1{1, "A", "one"});
        storage.replace(T1{2, "B", "two"});
        storage.replace(T1{3, "C", "three"});
        storage.replace(T1{4, "D", "one"});
        storage.replace(T1{5, "E", "two"});

        // SQL: SELECT a, sum(a) OVER (ORDER BY a ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t1
        auto frameSpec = rows(unbounded_preceding(), current_row());
        auto result = storage.select(columns(&T1::a, sum(&T1::a).over(order_by(&T1::a), frameSpec)));

        REQUIRE(result.size() == 5);
        REQUIRE(*std::get<1>(result[0]) == 1);
        REQUIRE(*std::get<1>(result[1]) == 3);
        REQUIRE(*std::get<1>(result[2]) == 6);
        REQUIRE(*std::get<1>(result[3]) == 10);
        REQUIRE(*std::get<1>(result[4]) == 15);
    }
    SECTION("FILTER + OVER") {
        storage.replace(T1{1, "A", "one"});
        storage.replace(T1{2, "B", "two"});
        storage.replace(T1{3, "C", "three"});
        storage.replace(T1{4, "D", "one"});
        storage.replace(T1{5, "E", "two"});

        // SQL: SELECT a, count(a) FILTER (WHERE c = 'one') OVER (ORDER BY a ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t1
        auto frameSpec = rows(unbounded_preceding(), current_row());
        auto result = storage.select(
            columns(&T1::a,
                    count(&T1::a).filter(where(c(&T1::c) == std::string("one"))).over(order_by(&T1::a), frameSpec)));

        REQUIRE(result.size() == 5);
        REQUIRE(std::get<1>(result[0]) == 1);
        REQUIRE(std::get<1>(result[1]) == 1);
        REQUIRE(std::get<1>(result[2]) == 1);
        REQUIRE(std::get<1>(result[3]) == 2);
        REQUIRE(std::get<1>(result[4]) == 2);
    }
    SECTION("NTILE") {
        storage.replace(T2{"a", "one"});
        storage.replace(T2{"a", "two"});
        storage.replace(T2{"a", "three"});
        storage.replace(T2{"b", "four"});
        storage.replace(T2{"c", "five"});
        storage.replace(T2{"c", "six"});

        // SQL: SELECT a, b, ntile(2) OVER (ORDER BY a) FROM t2
        auto rows = storage.select(columns(&T2::a, &T2::b, ntile(2).over(order_by(&T2::a))));

        REQUIRE(rows.size() == 6);
        // ntile(2) divides 6 rows into 2 groups: first 3 get 1, last 3 get 2
        REQUIRE(std::get<2>(rows[0]) == 1);
        REQUIRE(std::get<2>(rows[1]) == 1);
        REQUIRE(std::get<2>(rows[2]) == 1);
        REQUIRE(std::get<2>(rows[3]) == 2);
        REQUIRE(std::get<2>(rows[4]) == 2);
        REQUIRE(std::get<2>(rows[5]) == 2);
    }
}
