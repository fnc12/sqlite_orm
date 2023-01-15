#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("time") {
    auto storage = make_storage({});
    {
        auto rows = storage.select(time("12:00", "localtime"));
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().empty());
    }
    {
        auto rows = storage.select(time("12:00", "utc"));
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().empty());
    }
}

TEST_CASE("julianday") {
    struct Test {
        std::string text;
    };

    auto storage = make_storage({}, make_table("test", make_column("text", &Test::text)));
    storage.sync_schema();
    auto singleTestCase = [&storage](const std::string& arg, double expected) {
        {
            auto rows = storage.select(julianday(arg));
            REQUIRE(rows.size() == 1);
            REQUIRE((rows.front() - expected) < 0.001);  //  too much precision
        }
        {
            storage.insert(Test{arg});
            auto rows = storage.select(julianday(&Test::text));
            REQUIRE(rows.size() == 1);
            REQUIRE((rows.front() - expected) < 0.001);
            storage.remove_all<Test>();
        }
    };
    singleTestCase("2016-10-18", 2457679.5);
    singleTestCase("2016-10-18 16:45", 2457680.19791667);
    singleTestCase("2016-10-18 16:45:30", 2457680.19826389);
}

TEST_CASE("datetime") {
    auto storage = make_storage({});
    auto rows = storage.select(datetime("now"));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().empty());
}

TEST_CASE("date") {
    auto storage = make_storage({});
    auto rows = storage.select(date("now", "start of month", "+1 month", "-1 day"));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().empty());
}

TEST_CASE("strftime") {
    auto storage = make_storage({});
    auto rows = storage.select(strftime("%Y %m %d", "now"));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().empty());
}
