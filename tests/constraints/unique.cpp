#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Unique") {

    struct Contact {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string email;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Contact() = default;
        Contact(int id, std::string firstName, std::string lastName, std::string email) :
            id{id}, firstName{move(firstName)}, lastName{move(lastName)}, email{move(email)} {}
#endif
    };
    struct Shape {
        int id = 0;
        std::string backgroundColor;
        std::string foregroundColor;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Shape() = default;
        Shape(int id, std::string backgroundColor, std::string foregroundColor) :
            id{id}, backgroundColor{move(backgroundColor)}, foregroundColor{move(foregroundColor)} {}
#endif
    };
    struct List {
        int id = 0;
        std::unique_ptr<std::string> email;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        List() = default;
        List(int id, decltype(email) email) : id{id}, email{move(email)} {}
#endif
    };

    auto storage = make_storage(
        {},
        make_table("contacts",
                   make_column("contact_id", &Contact::id, primary_key()),
                   make_column("first_name", &Contact::firstName),
                   make_column("last_name", &Contact::lastName),
                   make_column("email", &Contact::email, unique())),
        make_table("shapes",
                   make_column("shape_id", &Shape::id, primary_key()),
                   make_column("background_color", &Shape::backgroundColor),
                   make_column("foreground_color", &Shape::foregroundColor),
                   sqlite_orm::unique(&Shape::backgroundColor, &Shape::foregroundColor)),
        make_table("lists", make_column("list_id", &List::id, primary_key()), make_column("email", &List::email)));
    storage.sync_schema();

    storage.insert(Contact{0, "John", "Doe", "john.doe@gmail.com"});

    try {
        storage.insert(Contact{0, "Johnny", "Doe", "john.doe@gmail.com"});
        REQUIRE(false);
    } catch(const std::system_error&) {
        //..
    } catch(...) {
        REQUIRE(false);
    }

    storage.insert(Shape{0, "red", "green"});
    storage.insert(Shape{0, "red", "blue"});
    try {
        storage.insert(Shape{0, "red", "green"});
        REQUIRE(false);
    } catch(const std::system_error&) {
        //..
    } catch(...) {
        REQUIRE(false);
    }

    std::vector<List> lists;
    lists.push_back(List{0, nullptr});
    lists.push_back(List{0, nullptr});
    storage.insert_range(lists.begin(), lists.end());
}
