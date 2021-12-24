#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator default") {
    internal::serializator_context_base context;
    {  //  int literal
        auto def = default_value(1);
        auto value = serialize(def, context);
        REQUIRE(value == "DEFAULT (1)");
    }
    {  //  string literal
        auto def = default_value("hi");
        auto value = serialize(def, context);
        REQUIRE(value == "DEFAULT ('hi')");
    }
    {  //  func
        auto def = default_value(datetime("now"));
        auto value = serialize(def, context);
        REQUIRE(value == "DEFAULT (DATETIME('now'))");
    }
}
