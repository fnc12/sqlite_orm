#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("triggers_basics") {
    struct TestInsert {
        int id;
        std::string text;
        int x = 0;
        int y = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        TestInsert() = default;
        TestInsert(int id, std::string text, int x, int y) : id{id}, text{std::move(text)}, x{x}, y{y} {}
#endif
    };
    struct TestUpdate {
        int id;
        std::string text;
        int x = 0;
        int y = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        TestUpdate() = default;
        TestUpdate(int id, std::string text, int x, int y) : id{id}, text{std::move(text)}, x{x}, y{y} {}
#endif
    };
    struct TestDelete {
        int id;
        std::string text;
        int x = 0;
        int y = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        TestDelete() = default;
        TestDelete(int id, std::string text, int x, int y) : id{id}, text{std::move(text)}, x{x}, y{y} {}
#endif
    };

    TestInsert test_insert{4, "test", 1, 2};
    TestUpdate test_update{4, "test", 1, 2};
    TestDelete test_delete{4, "test", 1, 2};
    auto storage = make_storage(
        "",
        make_trigger(
            "trigger_insert",
            after()
                .update_of(&TestInsert::x)
                .on<TestInsert>()
                .begin(insert(test_insert),
                       insert(into<TestInsert>(),
                              columns(&TestInsert::id, &TestInsert::text, &TestInsert::x, &TestInsert::y),
                              values(std::make_tuple(123, "HelloTrigger", 12, 13))),
                       insert(test_insert, columns(&TestInsert::id, &TestInsert::text, &TestInsert::x, &TestInsert::y)),
                       replace(TestInsert{8, "replace", 3, 4}),
                       replace(test_insert))
                .end()),
        make_trigger("trigger_update",
                     after()
                         .insert()
                         .on<TestUpdate>()
                         .begin(update(TestUpdate{test_update.id, "update", test_update.x, test_update.y}),
                                update_all(set(c(&TestUpdate::x) = 42), where(c(&TestUpdate::text) == "update")))
                         .end()),
        make_trigger(
            "trigger_delete",
            after()
                .insert()
                .on<TestDelete>()
                .begin(
                    // select(columns(&TestDelete::id), where(greater_than(&TestDelete::x, select(avg(&TestDelete::x))))), // TODO  near "(": syntax error: SQL logic error (expression is surronded by parenthesis and SQL returns an error for that)
                    remove<TestDelete>(test_delete.id),
                    remove_all<TestDelete>(where(c(&TestDelete::text) != "test")))
                .end()),
        make_table("test_insert",
                   make_column("sql_id", &TestInsert::id),
                   make_column("sql_text", &TestInsert::text),
                   make_column("sql_x", &TestInsert::x),
                   make_column("sql_y", &TestInsert::y),
                   primary_key(&TestInsert::id)),
        make_table("test_delete",
                   make_column("id", &TestDelete::id),
                   make_column("text", &TestDelete::text),
                   make_column("x", &TestDelete::x),
                   make_column("y", &TestDelete::y),
                   primary_key(&TestDelete::id)),
        make_table("test_update",
                   make_column("id", &TestUpdate::id),
                   make_column("text", &TestUpdate::text),
                   make_column("x", &TestUpdate::x),
                   make_column("y", &TestUpdate::y),
                   primary_key(&TestUpdate::id)));
    storage.sync_schema();

    SECTION("insert") {
        storage.insert(TestInsert{0, "SQLite trigger", 8, 2});
        REQUIRE(storage.count<TestInsert>() == 1);
        auto records = storage.get_all<TestInsert>();
        TestInsert t = records[0];
        t.x += 12;
        storage.update(t);
        REQUIRE(storage.count<TestInsert>() == 5);
    }
    SECTION("update") {
        storage.replace(test_update);
        REQUIRE(storage.count<TestUpdate>() == 1);
        auto records = storage.get_all<TestUpdate>();
        TestUpdate t = records[0];
        REQUIRE(t.text == "update");
        REQUIRE(t.x == 42);
    }
    SECTION("delete") {
        storage.replace(test_delete);
        storage.insert(TestDelete{0, "test", 1, 2});
        storage.insert(TestDelete{0, "test", 1, 2});
        storage.insert(TestDelete{0, "will be removed", 1, 2});
        REQUIRE(storage.count<TestDelete>() == 2);
    }
}
