#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator select constraints") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    std::string value;
    decltype(value) expected;
    SECTION("columns") {
        auto expression = columns(&User::id, &User::name);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "(\"id\", \"name\")";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "\"id\", \"name\"";
        }
        value = serialize(expression, context);
    }
    SECTION("into") {
        auto expression = into<User>();
        value = serialize(expression, context);
        expected = "INTO users";
    }
    SECTION("insert constraint") {
        SECTION("abort") {
            auto expression = or_abort();
            value = serialize(expression, context);
            expected = "OR ABORT";
        }
        SECTION("fail") {
            auto expression = or_fail();
            value = serialize(expression, context);
            expected = "OR FAIL";
        }
        SECTION("ignore") {
            auto expression = or_ignore();
            value = serialize(expression, context);
            expected = "OR IGNORE";
        }
        SECTION("replace") {
            auto expression = or_replace();
            value = serialize(expression, context);
            expected = "OR REPLACE";
        }
        SECTION("rollback") {
            auto expression = or_rollback();
            value = serialize(expression, context);
            expected = "OR ROLLBACK";
        }
    }
    SECTION("from") {
        SECTION("without alias") {
            auto expression = from<User>();
            value = serialize(expression, context);
            expected = "FROM 'users'";
        }
        SECTION("with alias") {
            auto expression = from<alias_u<User>>();
            value = serialize(expression, context);
            expected = "FROM 'users' 'u'";
        }
    }
    SECTION("function_call") {
        struct Func {
            bool operator()(int arg) const {
                return arg % 2;
            }

            static const char* name() {
                return "EVEN";
            }
        };
        auto expression = func<Func>(&User::id);
        value = serialize(expression, context);
        expected = "EVEN(\"id\")";
    }
    REQUIRE(value == expected);
}
