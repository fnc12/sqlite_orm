#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("triggers_basics") {
    struct Test {
        std::string text;
        int x = 0;
        int y = 0;
    };
    auto table_test = make_table(
        "test", make_column("sql_text", &Test::text),
        make_column("sql_x", &Test::x), make_column("sql_y", &Test::y)
    );
    Test a{"test_trigger", 1, 2};
    auto storage = make_storage(
        "test.sqlite",
        make_trigger(
            "test_trigger", trigger::after(), trigger::on_update(table_test, &Test::y, &Test::x, &Test::text),
//            sqlite_orm::update(a)
            insert(a)
        ),
        table_test
    );
    storage.sync_schema();

    {
        storage.insert(Test{"SQLite trigger", 8, 2});
        REQUIRE(storage.count<Test>() == 2);
    }
}
