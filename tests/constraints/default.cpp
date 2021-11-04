#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

//  appeared after #55
TEST_CASE("Default value") {
    struct User {
        int userId;
        std::string name;
        int age;
        std::string email;
    };

    auto filename = "test_db.sqlite";

    ::remove(filename);

    auto storage1 = make_storage(filename,
                                 make_table("User",
                                            make_column("Id", &User::userId, primary_key()),
                                            make_column("Name", &User::name),
                                            make_column("Age", &User::age)));
    storage1.sync_schema();
    storage1.remove_all<User>();

    auto emailColumn = make_column("Email", &User::email, default_value("example@email.com"));

    auto storage2 = make_storage(filename,
                                 make_table("User",
                                            make_column("Id", &User::userId, primary_key()),
                                            make_column("Name", &User::name),
                                            make_column("Age", &User::age),
                                            emailColumn));
    storage2.sync_schema();
    storage2.insert(User{0, "Tom", 15, ""});

    auto emailDefault = emailColumn.default_value();
    REQUIRE(emailDefault);
    auto& emailDefaultString = *emailDefault;
    REQUIRE(emailDefaultString == "'example@email.com'");
}

TEST_CASE("Default datetime") {
    struct Induction {
        std::string time;
    };

    auto storage = make_storage(
        {},
        make_table("induction",
                   make_column("timestamp", &Induction::time, default_value(datetime("now", "localtime")))));
    storage.sync_schema();
}

TEST_CASE("default value for string") {
    struct Contact {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string phone;
    };

    using namespace sqlite_orm;
    auto storage =
        make_storage({},
                     make_table("contacts",
                                make_column("contact_id", &Contact::id, primary_key()),
                                make_column("first_name", &Contact::firstName, default_value<std::string>("")),
                                make_column("last_name", &Contact::lastName, default_value<std::string>("")),
                                make_column("phone", &Contact::phone)));
    storage.sync_schema();
}
