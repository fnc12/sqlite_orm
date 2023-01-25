#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("issue525") {
    struct User {
        int id;
        std::string firstName;
        std::string lastName;
        int birthDate;
        std::unique_ptr<std::string> imageUrl;
        int typeId;
    };

    struct UserType {
        int id;
        std::string name;
    };

    auto filename = "db1.sqlite";
    auto storage = make_storage(filename,
                                make_table("users",
                                           make_column("id", &User::id, primary_key().autoincrement()),
                                           make_column("first_name", &User::firstName),
                                           make_column("last_name", &User::lastName),
                                           make_column("birth_date", &User::birthDate),
                                           make_column("image_url", &User::imageUrl),
                                           make_column("type_id", &User::typeId)),
                                make_table("user_types",
                                           make_column("id", &UserType::id, primary_key().autoincrement()),
                                           make_column("name", &UserType::name, default_value("name_placeholder"))));

    storage.sync_schema();
}
