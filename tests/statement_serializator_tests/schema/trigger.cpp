#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator trigger") {
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
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};
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
        expected =
            "CREATE TRIGGER IF NOT EXISTS 'validate_email_before_insert_leads' BEFORE INSERT ON 'leads' BEGIN SELECT "
            "CASE WHEN NOT  (NEW.\"email\" LIKE '%_@__%.__%' ) THEN RAISE(ABORT, 'Invalid email address') END; END";
    }
    REQUIRE(value == expected);
}