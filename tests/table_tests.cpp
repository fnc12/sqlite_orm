#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("table") {
    {
        struct Contact {
            int id = 0;
            std::string firstName;
            std::string lastName;
            int countryCode = 0;
            std::string phoneNumber;
            int visitsCount = 0;
        };
        auto table = make_table("contacts",
                                make_column("contact_id", &Contact::id, primary_key(), autoincrement()),
                                make_column("first_name", &Contact::firstName),
                                make_column("last_name", &Contact::lastName),
                                make_column("country_code", &Contact::countryCode),
                                make_column("phone_number", &Contact::phoneNumber),
                                make_column("visits_count", &Contact::visitsCount));
        REQUIRE(table.find_column_name(&Contact::id) == "contact_id");
        REQUIRE(table.find_column_name(&Contact::firstName) == "first_name");
        REQUIRE(table.find_column_name(&Contact::lastName) == "last_name");
        REQUIRE(table.find_column_name(&Contact::countryCode) == "country_code");
        REQUIRE(table.find_column_name(&Contact::phoneNumber) == "phone_number");
        REQUIRE(table.find_column_name(&Contact::visitsCount) == "visits_count");
    }
    {
        /*struct Contact {
        private:
            int _id = 0;
            std::string _firstName;
            std::string _lastName;
            int _countryCode = 0;
            std::string _phoneNumber;
            int _visitsCount = 0;
        
        public:
            int id() const {
                return this->_id;
            }
            
            void setId(int value) {
                this->_id = value;
            }
        };
        auto table = make_table("contacts",
                                make_column("contact_id", &Contact::id, primary_key(), autoincrement()),
                                make_column("first_name", &Contact::firstName),
                                make_column("last_name", &Contact::lastName),
                                make_column("country_code", &Contact::countryCode),
                                make_column("phone_number", &Contact::phoneNumber),
                                make_column("visits_count", &Contact::visitsCount));
        REQUIRE(table.find_column_name(&Contact::id) == "contact_id");
        REQUIRE(table.find_column_name(&Contact::firstName) == "first_name");
        REQUIRE(table.find_column_name(&Contact::lastName) == "last_name");
        REQUIRE(table.find_column_name(&Contact::countryCode) == "country_code");
        REQUIRE(table.find_column_name(&Contact::phoneNumber) == "phone_number");
        REQUIRE(table.find_column_name(&Contact::visitsCount) == "visits_count");*/
    }
}
