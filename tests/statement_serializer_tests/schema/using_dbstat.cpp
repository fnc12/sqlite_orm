#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#pragma warning(default : 4996)
using namespace sqlite_orm;

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("statement_serializer dbstat") {
    struct mystat : dbstat {
        // A clever way of defining and using explicit column pointers for hidden `dbstat` member fields
        using hidden = dbstat::hidden_columns_for<mystat>;
    };
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_reference auto mystat_table = c<mystat>();
#endif

    std::string value;
    std::string expected;

    SECTION("create") {
        using db_objects_t = internal::db_objects_tuple<>;
        const db_objects_t dbObjects{};
        using context_t = internal::serializer_context<db_objects_t>;
        context_t context{dbObjects};
        SECTION("default") {
            auto expression = make_virtual_table<mystat>("mystat", using_dbstat());
            value = serialize(expression, context);
            expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "mystat" USING "dbstat")";
        }
        SECTION("table value") {
            auto expression = make_virtual_table<mystat>("mystat", using_dbstat("main"));
            value = serialize(expression, context);
            expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "mystat" USING "dbstat"('main'))";
        }
#if SQLITE_VERSION_NUMBER >= 3031000
        SECTION("table value 2") {
            auto expression = make_virtual_table<mystat>("mystat", using_dbstat("main", true));
            value = serialize(expression, context);
            expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "mystat" USING "dbstat"('main', 1))";
        }
#endif
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        SECTION("table reference") {
            auto expression = make_virtual_table<mystat_table>("mystat", using_dbstat());
            value = serialize(expression, context);
            expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "mystat" USING "dbstat")";
        }
#endif
    }
    SECTION("select") {
        auto table = make_virtual_table<mystat>("dbstat", using_dbstat());
        using table_type = decltype(table);
        using db_objects_t = internal::db_objects_tuple<table_type>;
        const db_objects_t dbObjects{table};
        using context_t = internal::serializer_context<db_objects_t>;
        context_t context{dbObjects};
        context.use_parentheses = false;
        SECTION("asterisk, without hidden columns") {
            auto expression = select(asterisk<mystat>(true));
            value = serialize(expression, context);
            expected =
                R"(SELECT "dbstat"."name", "dbstat"."path", "dbstat"."pageno", "dbstat"."pagetype", "dbstat"."ncell", "dbstat"."payload", "dbstat"."unused", "dbstat"."mx_payload", "dbstat"."pgoffset", "dbstat"."pgsize" FROM "dbstat")";
        }
        SECTION("object, without hidden columns") {
            auto expression = select(object<mystat>(true));
            value = serialize(expression, context);
            expected =
                R"(SELECT "dbstat"."name", "dbstat"."path", "dbstat"."pageno", "dbstat"."pagetype", "dbstat"."ncell", "dbstat"."payload", "dbstat"."unused", "dbstat"."mx_payload", "dbstat"."pgoffset", "dbstat"."pgsize" FROM "dbstat")";
        }
        SECTION("with hidden column in query") {
            auto expression = select(columns(column<mystat>(&dbstat::name), column<mystat>(&dbstat::hidden::schema)),
                                     where(column<mystat>(&dbstat::hidden::schema) == "main"));
            value = serialize(expression, context);
            expected = R"(SELECT "dbstat"."name", "dbstat"."schema" FROM "dbstat" WHERE ("dbstat"."schema" = 'main'))";
        }
        SECTION("with hidden column in query v2") {
            auto expression = select(columns(column<mystat>(&dbstat::name), mystat::hidden::schema_column),
                                     where(mystat::hidden::schema_column == "main"));
            value = serialize(expression, context);
            expected = R"(SELECT "dbstat"."name", "dbstat"."schema" FROM "dbstat" WHERE ("dbstat"."schema" = 'main'))";
        }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        SECTION("with hidden column in query [table reference]") {
            auto expression = select(columns(mystat_table->*&dbstat::name, mystat_table->*&dbstat::hidden::schema),
                                     where(mystat_table->*&dbstat::hidden::schema == "main"));
            value = serialize(expression, context);
            expected = R"(SELECT "dbstat"."name", "dbstat"."schema" FROM "dbstat" WHERE ("dbstat"."schema" = 'main'))";
        }
#endif
    }
    REQUIRE(value == expected);
}
#endif
