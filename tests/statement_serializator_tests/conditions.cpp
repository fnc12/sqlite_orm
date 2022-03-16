#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator conditions") {
    SECTION("order by") {
        internal::serializator_context_base context;

        SECTION("dump") {
            context.replace_bindable_with_question = false;
            auto ast = order_by(2);
            auto value = serialize(ast, context);
            REQUIRE(value == "ORDER BY 2");
        }
        SECTION("bindable") {
            context.replace_bindable_with_question = true;
            auto ast = order_by(2);
            auto value = serialize(ast, context);
            REQUIRE(value == "ORDER BY ?");
        }
    }
    SECTION("using") {
        struct User {
            int64 id;
        };

        auto t1 = make_table("user", make_column("id", &User::id));
        auto storage = internal::storage_impl<decltype(t1)>{t1};
        using storage_impl = decltype(storage);

        internal::serializator_context<storage_impl> ctx{storage};

        SECTION("using column") {
            auto ast = using_(&User::id);
            auto value = serialize(ast, ctx);
            REQUIRE(value == R"(USING ("id"))");
        }
        SECTION("using explicit column") {
            auto ast = using_(column<User>(&User::id));
            auto value = serialize(ast, ctx);
            REQUIRE(value == R"(USING ("id"))");
        }
    }
}
