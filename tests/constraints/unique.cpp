#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include "../catch_matchers.h"

using namespace sqlite_orm;

TEST_CASE("Unique") {
#if SQLITE_VERSION_NUMBER >= 3037002
    const ErrorCodeExceptionMatcher uniqueExceptionMatcher(sqlite_errc(SQLITE_CONSTRAINT_UNIQUE));
#else
    const ErrorCodeExceptionMatcher uniqueExceptionMatcher(sqlite_errc(SQLITE_CONSTRAINT));
#endif

    struct Contact {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string email;
    };
    struct Shape {
        int id = 0;
        std::string backgroundColor;
        std::string foregroundColor;
    };
    struct List {
        int id = 0;
        std::unique_ptr<std::string> email;
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

    REQUIRE_THROWS_MATCHES(storage.insert(Contact{0, "Johnny", "Doe", "john.doe@gmail.com"}),
                           std::system_error,
                           uniqueExceptionMatcher);

    storage.insert(Shape{0, "red", "green"});
    storage.insert(Shape{0, "red", "blue"});
    REQUIRE_THROWS_MATCHES(storage.insert(Shape{0, "red", "green"}), std::system_error, uniqueExceptionMatcher);

    std::vector<List> lists(2);
    REQUIRE_NOTHROW(storage.insert_range(lists.begin(), lists.end()));
}
