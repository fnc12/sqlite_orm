#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer update") {
    using internal::serialize;
    struct A {
        int address = 0;
        int type = 0;
        int index = 0;
        double value = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        A() = default;
        A(int address, int type, int index, double value) : address{address}, type{type}, index{index}, value{value} {}
#endif
    };
    std::string value;
    decltype(value) expected;
    SECTION("primary key") {
        SECTION("column") {
            auto table = make_table("table",
                                    make_column("address", &A::address, primary_key()),
                                    make_column("type", &A::type),
                                    make_column("idx", &A::index),
                                    make_column("value", &A::value));
            using schema_objects_t = internal::schema_objects<decltype(table)>;
            auto storageImpl = schema_objects_t{table};
            using context_t = internal::serializer_context<schema_objects_t>;
            context_t context{storageImpl};
            SECTION("update") {
                SECTION("with question marks") {
                    context.replace_bindable_with_question = true;
                    expected = R"(UPDATE "table" SET "type" = ?, "idx" = ?, "value" = ? WHERE "address" = ?)";
                }
                SECTION("without question marks") {
                    context.replace_bindable_with_question = false;
                    expected = R"(UPDATE "table" SET "type" = 2, "idx" = 3, "value" = 4 WHERE "address" = 1)";
                }

                A object{1, 2, 3, 4};
                auto expression = update(object);
                value = serialize(expression, context);
            }
            SECTION("update_all") {
                auto expression = update_all(set(assign(&A::value, 5)), where(is_equal(&A::address, 1)));
                SECTION("with question marks") {
                    context.replace_bindable_with_question = true;
                    expected = R"(UPDATE "table" SET "value" = ? WHERE (("address" = ?)))";
                }
                SECTION("without question marks") {
                    context.replace_bindable_with_question = false;
                    expected = R"(UPDATE "table" SET "value" = 5 WHERE (("address" = 1)))";
                }
                value = serialize(expression, context);
            }
        }
        SECTION("table") {
            auto table = make_table("table",
                                    make_column("address", &A::address),
                                    make_column("type", &A::type),
                                    make_column("idx", &A::index),
                                    make_column("value", &A::value),
                                    primary_key(&A::address));
            using schema_objects_t = internal::schema_objects<decltype(table)>;
            auto storageImpl = schema_objects_t{table};
            using context_t = internal::serializer_context<schema_objects_t>;
            context_t context{storageImpl};
            SECTION("with question marks") {
                context.replace_bindable_with_question = true;
                expected = R"(UPDATE "table" SET "type" = ?, "idx" = ?, "value" = ? WHERE "address" = ?)";
            }
            SECTION("without question marks") {
                context.replace_bindable_with_question = false;
                expected = R"(UPDATE "table" SET "type" = 2, "idx" = 3, "value" = 4 WHERE "address" = 1)";
            }

            A object{1, 2, 3, 4};
            auto expression = update(object);
            value = serialize(expression, context);
        }
    }
    SECTION("composite key 2") {
        auto table = make_table("table",
                                make_column("address", &A::address),
                                make_column("type", &A::type),
                                make_column("idx", &A::index),
                                make_column("value", &A::value),
                                primary_key(&A::address, &A::type));
        using schema_objects_t = internal::schema_objects<decltype(table)>;
        auto storageImpl = schema_objects_t{table};
        using context_t = internal::serializer_context<schema_objects_t>;
        context_t context{storageImpl};
        SECTION("with question marks") {
            context.replace_bindable_with_question = true;
            expected = R"(UPDATE "table" SET "idx" = ?, "value" = ? WHERE "address" = ? AND "type" = ?)";
        }
        SECTION("without question marks") {
            context.replace_bindable_with_question = false;
            expected = R"(UPDATE "table" SET "idx" = 3, "value" = 4 WHERE "address" = 1 AND "type" = 2)";
        }

        A object{1, 2, 3, 4};
        auto expression = update(object);
        value = serialize(expression, context);
    }
    SECTION("composite key 3") {
        auto table = make_table("table",
                                make_column("address", &A::address),
                                make_column("type", &A::type),
                                make_column("idx", &A::index),
                                make_column("value", &A::value),
                                primary_key(&A::address, &A::type, &A::index));
        using schema_objects_t = internal::schema_objects<decltype(table)>;
        auto storageImpl = schema_objects_t{table};
        using context_t = internal::serializer_context<schema_objects_t>;
        context_t context{storageImpl};
        SECTION("question marks") {
            context.replace_bindable_with_question = true;
            expected = R"(UPDATE "table" SET "value" = ? WHERE "address" = ? AND "type" = ? AND "idx" = ?)";
        }
        SECTION("no question marks") {
            context.replace_bindable_with_question = false;
            expected = R"(UPDATE "table" SET "value" = 4 WHERE "address" = 1 AND "type" = 2 AND "idx" = 3)";
        }

        A object{1, 2, 3, 4};
        auto expression = update(object);
        value = serialize(expression, context);
    }
    REQUIRE(value == expected);
}
