#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

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

    auto statement =
        update_all(set(c(&Contact::phone) = select(&Customer::phone, from<Customer>(), where(c(&Customer::id) == 1))));
    auto value = serialize(statement, context);
    decltype(value) expected =
        R"(UPDATE "contacts" SET "phone" = (SELECT "customers"."Phone" FROM "customers" WHERE (("CustomerId" = 1))))";
    REQUIRE(value == expected);
}
