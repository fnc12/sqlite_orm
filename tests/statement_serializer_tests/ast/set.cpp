#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("set") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    std::string value;
    std::string expected;
    SECTION("one item") {
        SECTION("static") {
            auto expression = set(assign(&User::id, 5));
            value = internal::serialize(expression, context);
        }
        SECTION("dynamic") {
            auto storage = make_storage("", table);
            auto expression = dynamic_set(storage);
            expression.push_back(assign(&User::id, 5));
            SECTION("empty") {
                //..
            }
            SECTION("clear and push_back") {
                expression.clear();
                expression.push_back(assign(&User::id, 5));
            }
            value = internal::serialize(expression, context);
        }
        expected = "SET \"id\" = 5";
    }
    SECTION("two items") {
        SECTION("static") {
            auto expression = set(assign(&User::id, 5), assign(&User::name, "ototo"));
            value = internal::serialize(expression, context);
        }
        SECTION("dynamic") {
            auto storage = make_storage("", table);
            auto expression = dynamic_set(storage);
            expression.push_back(assign(&User::id, 5));
            expression.push_back(assign(&User::name, "ototo"));
            SECTION("empty") {
                //..
            }
            SECTION("clear and push_back") {
                expression.clear();
                expression.push_back(assign(&User::id, 5));
                expression.push_back(assign(&User::name, "ototo"));
            }
            value = internal::serialize(expression, context);
        }
        expected = "SET \"id\" = 5, \"name\" = 'ototo'";
    }
    REQUIRE(value == expected);
}
