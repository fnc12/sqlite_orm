#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer trigger") {
    using internal::serialize;
    struct Lead {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string email;
        std::string phone;
    };
    auto table = make_table("leads",
                            make_column("id", &Lead::id, primary_key()),
                            make_column("first_name", &Lead::firstName),
                            make_column("last_name", &Lead::lastName),
                            make_column("email", &Lead::email),
                            make_column("phone", &Lead::phone));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    std::string value;
    decltype(value) expected;
    SECTION("without for each row") {
        auto expression = make_trigger("validate_email_before_insert_leads",
                                       before()
                                           .insert()
                                           .on<Lead>()
                                           .begin(select(case_<int>()
                                                             .when(not like(new_(&Lead::email), "%_@__%.__%"),
                                                                   then(raise_abort("Invalid email address")))
                                                             .end()))
                                           .end());
        value = serialize(expression, context);
        expected = R"(CREATE TRIGGER "validate_email_before_insert_leads" BEFORE INSERT ON "leads" BEGIN SELECT )"
                   R"(CASE WHEN NOT NEW."email" LIKE '%_@__%.__%' THEN RAISE(ABORT, 'Invalid email address') END; END)";
    }
    SECTION("for each row") {
        auto expression =
            make_trigger("trg", after().delete_().on<Lead>().for_each_row().begin(select(old(&Lead::id))).end());
        value = serialize(expression, context);
        expected = R"(CREATE TRIGGER "trg" AFTER DELETE ON "leads" FOR EACH ROW BEGIN SELECT OLD."id"; END)";
    }
    SECTION("when") {
        auto expression = make_trigger(
            "trg",
            instead_of().update().on<Lead>().when(is_equal(new_(&Lead::phone), "")).begin(select(1)).end());
        value = serialize(expression, context);
        expected = R"(CREATE TRIGGER "trg" INSTEAD OF UPDATE ON "leads" WHEN NEW."phone" = '' )"
                   R"(BEGIN SELECT 1; END)";
    }
    SECTION("update of") {
        auto expression =
            make_trigger("trg", before().update_of(&Lead::email, &Lead::phone).on<Lead>().begin(select(1)).end());
        value = serialize(expression, context);
        expected = R"(CREATE TRIGGER "trg" BEFORE UPDATE OF "email", "phone" ON "leads" BEGIN SELECT 1; END)";
    }
    REQUIRE(value == expected);
}
