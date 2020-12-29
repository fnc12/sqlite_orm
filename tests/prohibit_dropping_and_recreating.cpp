#define SQLITE_ORM_PROHIBIT_DROPPING_AND_RECREATING
#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("prohibit_dropping_and_recreating") {
    using Catch::Matchers::UnorderedEquals;
    struct User {
        int id = 0;
        std::string name;
        int age = 0;
        std::unique_ptr<int> ageNullable;

        User() = default;

        User(int id_) : id(id_) {}

        User(int id_, std::string name_) : id(id_), name(move(name_)) {}

        User(int id_, int age_) : id(id_), age(age_) {}

        User(const User& other) :
            id(other.id), name(other.name), age(other.age),
            ageNullable(other.ageNullable ? std::make_unique<int>(*other.ageNullable) : nullptr) {}

        bool operator==(const User& other) const {
            return this->id == other.id && this->name == other.name && this->age == other.age;
        }
    };
    auto storagePath = "prohibit_dropping_and_recreating.sqlite";
    std::string tableName = "users";
    struct {
        const std::string id = "id";
        const std::string name = "name";
        const std::string age = "age";
    } columnNames;
    ::remove(storagePath);

    /* ------------------------------------------------------------------------------------------- */
    auto isStorageEmpty = [&]() {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.name, &User::name)));
        storage.sync_schema();

        auto users = storage.get_all<User>();
        return users.empty();
    };
    /* ------------------------------------------------------------------------------------------- */

    {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.name, &User::name)));
        auto syncSchemaSimulateRes = storage.sync_schema_simulate(true);
        auto syncSchemaRes = storage.sync_schema(true);
        REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
        decltype(syncSchemaSimulateRes) expected{
            {tableName, sync_schema_result::new_table_created},
        };
        REQUIRE(syncSchemaSimulateRes == expected);

        storage.replace(User{1, "Alex"});
        storage.replace(User{2, "Michael"});
    }
    SECTION("remove name column") {
        auto storage =
            make_storage(storagePath, make_table(tableName, make_column(columnNames.id, &User::id, primary_key())));
        auto syncSchemaSimulateRes = storage.sync_schema_simulate();
        try {
            storage.sync_schema();
            REQUIRE(false);
        } catch(const std::system_error& e) {
            //..
        } catch(...) {
            REQUIRE(false);
        }
        decltype(syncSchemaSimulateRes) expected{
            {tableName, sync_schema_result::dropped_and_recreated},
        };
        REQUIRE(syncSchemaSimulateRes == expected);
        REQUIRE(!isStorageEmpty());
    }
    SECTION("replace a column with no default value") {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.age, &User::age)));
        std::map<std::string, sync_schema_result> syncSchemaSimulateRes;
        SECTION("preserve = true") {
            syncSchemaSimulateRes = storage.sync_schema_simulate(true);
            try {
                storage.sync_schema(true);
                REQUIRE(false);
            } catch(const std::system_error& e) {
                //..
            } catch(...) {
                REQUIRE(false);
            }
        }
        SECTION("preserve = false") {
            syncSchemaSimulateRes = storage.sync_schema_simulate();
            try {
                storage.sync_schema();
                REQUIRE(false);
            } catch(const std::system_error& e) {
                //..
            } catch(...) {
                REQUIRE(false);
            }
        }
        decltype(syncSchemaSimulateRes) expected{
            {tableName, sync_schema_result::dropped_and_recreated},
        };
        REQUIRE(syncSchemaSimulateRes == expected);
        REQUIRE(!isStorageEmpty());
    }
    SECTION("replace a column with default value") {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.age, &User::age, default_value(-1))));
        auto syncSchemaSimulateRes = storage.sync_schema_simulate();
        try {
            storage.sync_schema();
            REQUIRE(false);
        } catch(const std::system_error& e) {
            //..
        } catch(...) {
            REQUIRE(false);
        }
        decltype(syncSchemaSimulateRes) expected{
            {tableName, sync_schema_result::dropped_and_recreated},
        };
        REQUIRE(syncSchemaSimulateRes == expected);
        REQUIRE(!isStorageEmpty());
    }
    SECTION("replace a column with null") {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.age, &User::ageNullable)));
        auto syncSchemaSimulateRes = storage.sync_schema_simulate();
        try {
            storage.sync_schema();
            REQUIRE(false);
        } catch(const std::system_error& e) {
            //..
        } catch(...) {
            REQUIRE(false);
        }

        decltype(syncSchemaSimulateRes) expected{
            {tableName, sync_schema_result::dropped_and_recreated},
        };
        REQUIRE(syncSchemaSimulateRes == expected);
        REQUIRE(!isStorageEmpty());
    }
}
