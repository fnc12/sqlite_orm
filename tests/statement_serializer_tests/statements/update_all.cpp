#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer update_all") {
    using internal::serialize;
    struct Contact {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string phone;
    };

    struct Customer {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string company;
        std::string address;
        std::string city;
        std::string state;
        std::string country;
        std::string postalCode;
        std::string phone;
        std::unique_ptr<std::string> fax;
        std::string email;
        int supportRepId = 0;
    };
    auto contactsTable = make_table("contacts",
                                    make_column("contact_id", &Contact::id, primary_key()),
                                    make_column("first_name", &Contact::firstName),
                                    make_column("last_name", &Contact::lastName),
                                    make_column("phone", &Contact::phone));
    auto customersTable = make_table("customers",
                                     make_column("CustomerId", &Customer::id, primary_key()),
                                     make_column("FirstName", &Customer::firstName),
                                     make_column("LastName", &Customer::lastName),
                                     make_column("Company", &Customer::company),
                                     make_column("Address", &Customer::address),
                                     make_column("City", &Customer::city),
                                     make_column("State", &Customer::state),
                                     make_column("Country", &Customer::country),
                                     make_column("PostalCode", &Customer::postalCode),
                                     make_column("Phone", &Customer::phone),
                                     make_column("Fax", &Customer::fax),
                                     make_column("Email", &Customer::email),
                                     make_column("SupportRepId", &Customer::supportRepId));
    using db_objects_t = internal::db_objects_tuple<decltype(contactsTable), decltype(customersTable)>;
    auto dbObjects = db_objects_t{contactsTable, customersTable};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    std::string value;
    std::string expected;
    SECTION("select") {
        auto expression = update_all(set(c(&Contact::phone) = select(&Customer::phone, where(c(&Customer::id) == 1))),
                                     where(c(&Contact::id) == 1));
        value = serialize(expression, context);
        expected =
            R"(UPDATE "contacts" SET "phone" = (SELECT "customers"."Phone" FROM "customers" WHERE ("customers"."CustomerId" = 1)) WHERE ("contact_id" = 1))";
    }
#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("With clause") {
        constexpr orm_cte_moniker auto data = "data"_cte;
        constexpr auto cteExpression = cte<data>().as(select(&Customer::phone, where(c(&Customer::id) == 1)));
        auto dbObjects2 = internal::db_objects_cat(dbObjects, internal::make_cte_table(dbObjects, cteExpression));
        using context_t = internal::serializer_context<decltype(dbObjects2)>;
        context_t context2{dbObjects2};

        auto expression =
            with(cteExpression,
                 update_all(set(c(&Contact::phone) = select(data->*&Customer::phone)), where(c(&Contact::id) == 1)));

        value = serialize(expression, context2);
        expected =
            R"(WITH "data"("Phone") AS (SELECT "customers"."Phone" FROM "customers" WHERE ("customers"."CustomerId" = 1)) UPDATE "contacts" SET "phone" = (SELECT "data"."Phone" FROM "data") WHERE ("contact_id" = 1))";
    }
#endif
#endif
    REQUIRE(value == expected);
}
