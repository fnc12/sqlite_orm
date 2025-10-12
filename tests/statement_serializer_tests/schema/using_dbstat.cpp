#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("statement_serializer dbstat") {
    struct mystat : dbstat {};
    constexpr auto mystat_table = c<mystat>();

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
        SECTION("table value 1") {
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
    SECTION("expressions") {
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
            auto expression = select(columns(mystat_table->*&dbstat::name, mystat_table->*&dbstat::hidden::schema),
                                     where(mystat_table->*&dbstat::hidden::schema == "main"));
            value = serialize(expression, context);
            expected = R"(SELECT "dbstat"."name", "dbstat"."schema" FROM "dbstat" WHERE ("dbstat"."schema" = 'main'))";
        }
        SECTION("with hidden column in query v2") {
            auto expression = select(columns(mystat_table->*&dbstat::name, mystat_table->*&dbstat::hidden::schema),
                                     where(mystat_table->*&dbstat::hidden::schema == "main"));
            value = serialize(expression, context);
            expected = R"(SELECT "dbstat"."name", "dbstat"."schema" FROM "dbstat" WHERE ("dbstat"."schema" = 'main'))";
        }
        SECTION("table-valued function 0") {
            value = serialize(from(mystat_table()), context);
            expected = R"(FROM "dbstat"())";
        }
        SECTION("table-valued function 1") {
            value = serialize(from(mystat_table("main")), context);
            expected = R"(FROM "dbstat"('main'))";
        }
#if SQLITE_VERSION_NUMBER >= 3031000
        SECTION("table-valued function 2") {
            value = serialize(from(mystat_table("main", true)), context);
            expected = R"(FROM "dbstat"('main', 1))";
        }
#endif
    }
    REQUIRE(value == expected);
}
#endif
