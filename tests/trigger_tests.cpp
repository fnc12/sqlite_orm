#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("triggers_basics") {
    struct Test {
        int id;
        std::string text;
        int x = 0;
        int y = 0;
    };
    auto table_test = make_table("test",
                                 make_column("sql_id", &Test::id),
                                 make_column("sql_text", &Test::text),
                                 make_column("sql_x", &Test::x),
                                 make_column("sql_y", &Test::y),
                                 primary_key(&Test::id));

    Test a{0, "test", 1, 2};
    auto storage = make_storage(
        "",
        make_trigger("trigger2", after().update_of(&Test::y, &Test::x, &Test::text).on<Test>().begin(insert(a)).end()),
        table_test);
    storage.sync_schema();

    {
        storage.insert(Test{0, "SQLite trigger", 8, 2});
        REQUIRE(storage.count<Test>() == 1);
        auto records = storage.get_all<Test>();
        REQUIRE(records.size() == 1);
        Test t = records[0];
        t.x += 12;
        storage.update(t);
        REQUIRE(storage.count<Test>() == 2);
    }
}
