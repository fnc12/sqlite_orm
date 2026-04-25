#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <cstdio>  //  std::remove

using namespace sqlite_orm;

TEST_CASE("triggers_basics") {
    struct TestInsert {
        int id = 0;
        std::string text;
        int x = 0;
        int y = 0;
    };
    struct TestUpdate {
        int id = 0;
        std::string text;
        int x = 0;
        int y = 0;
    };
    struct TestDelete {
        int id = 0;
        std::string text;
        int x = 0;
        int y = 0;
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

TEST_CASE("trigger_names") {
    auto storagePath = "trigger_names.sqlite";
    struct X {
        int test = 0;
    };

    {
        auto storage = make_storage(
            storagePath,
            make_trigger("trigger1", after().insert().on<X>().begin(update_all(set(c(&X::test) = 1))).end()),
            make_trigger("trigger2", after().insert().on<X>().begin(update_all(set(c(&X::test) = 2))).end()),
            make_table("x", make_column("test", &X::test)));
        storage.sync_schema();
    }
    {
        auto storage = make_storage(
            storagePath,
            make_trigger("trigger2", after().insert().on<X>().begin(update_all(set(c(&X::test) = 2))).end()),
            make_trigger("trigger3", after().insert().on<X>().begin(update_all(set(c(&X::test) = 3))).end()),
            make_table("x", make_column("test", &X::test)));
        storage.sync_schema();

        auto trigger_names = storage.trigger_names();
        REQUIRE_THAT(trigger_names,
                     Catch::Matchers::UnorderedEquals(std::vector<std::string>{"trigger1", "trigger2", "trigger3"}));
    }
}

TEST_CASE("issue1429") {
    struct Lead {
        int id = 0;
        std::string name;
        std::string email;
    };

    auto storagePath = "issue1429.sqlite";
    std::remove(storagePath);

    // first: create storage with trigger checking "name" column
    {
        auto storage = make_storage(storagePath,
                                    make_trigger("validate_email_before_insert_leads",
                                                 before()
                                                     .insert()
                                                     .on<Lead>()
                                                     .begin(select(case_<int>()
                                                                       .when(not like(new_(&Lead::name), "%_@__%.__%"),
                                                                             then(raise_abort("Invalid email address")))
                                                                       .end()))
                                                     .end()),
                                    make_table("leads",
                                               make_column("id", &Lead::id, primary_key()),
                                               make_column("name", &Lead::name),
                                               make_column("email", &Lead::email)));
        auto syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("validate_email_before_insert_leads") == sync_schema_result::new_table_created);

        // second sync should report already_in_sync
        syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("validate_email_before_insert_leads") == sync_schema_result::already_in_sync);
    }
    // second: create storage with trigger checking "email" column instead
    {
        auto storage = make_storage(storagePath,
                                    make_trigger("validate_email_before_insert_leads",
                                                 before()
                                                     .insert()
                                                     .on<Lead>()
                                                     .begin(select(case_<int>()
                                                                       .when(not like(new_(&Lead::email), "%_@__%.__%"),
                                                                             then(raise_abort("Invalid email address")))
                                                                       .end()))
                                                     .end()),
                                    make_table("leads",
                                               make_column("id", &Lead::id, primary_key()),
                                               make_column("name", &Lead::name),
                                               make_column("email", &Lead::email)));

        // simulate should detect the change
        auto simulateResult = storage.sync_schema_simulate();
        REQUIRE(simulateResult.at("validate_email_before_insert_leads") == sync_schema_result::dropped_and_recreated);

        // sync should update the trigger
        auto syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("validate_email_before_insert_leads") == sync_schema_result::dropped_and_recreated);

        // verify trigger was updated: inserting a row with invalid email should fail
        REQUIRE_THROWS(storage.insert(Lead{0, "John", "not_an_email"}));

        // valid email should succeed
        REQUIRE_NOTHROW(storage.insert(Lead{0, "John", "john@example.com"}));

        // after update, second sync should be already_in_sync
        syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("validate_email_before_insert_leads") == sync_schema_result::already_in_sync);
    }

    std::remove(storagePath);
}

TEST_CASE("issue1280") {
    struct X {
        int test = 0;
    };
    auto storage = make_storage(
        "",
        make_trigger("table_insert_InsertTest", after().insert().on<X>().begin(update_all(set(c(&X::test) = 5))).end()),
        make_table("x", make_column("test", &X::test)));
    storage.sync_schema();
    storage.sync_schema_simulate();
}
