#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("check") {
    SECTION("table level") {
        struct Contact {
            int id = 0;
            std::string firstName;
            std::string lastName;
            std::unique_ptr<std::string> email;
            std::string phone;
        };
        auto storage = make_storage({},
                                    make_table("contacts",
                                               make_column("contact_id", &Contact::id, primary_key()),
                                               make_column("first_name", &Contact::firstName),
                                               make_column("last_name", &Contact::lastName),
                                               make_column("email", &Contact::email),
                                               make_column("phone", &Contact::phone),
                                               check(length(&Contact::phone) >= 10)));
        storage.sync_schema();
    }
    SECTION("column level") {
        struct Book {
            int id = 0;
            std::string name;
            std::string pubName;
            int price = 0;
        };
        SECTION(">") {
            auto storage = make_storage({},
                                        make_table("BOOK",
                                                   make_column("Book_id", &Book::id, primary_key()),
                                                   make_column("Book_name", &Book::name),
                                                   make_column("Pub_name", &Book::pubName),
                                                   make_column("PRICE", &Book::price, check(c(&Book::price) > 0))));
            storage.sync_schema();
        }
        SECTION("like") {
            auto storage = make_storage({},
                                        make_table("BOOK",
                                                   make_column("Book_id", &Book::id, primary_key()),
                                                   make_column("Book_name", &Book::name),
                                                   make_column("Pub_name", &Book::pubName),
                                                   make_column("PRICE", &Book::price, check(c(&Book::price) > 0)),
                                                   check(like(&Book::pubName, "M%"))));
            storage.sync_schema();
        }
    }
}
