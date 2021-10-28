#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <iostream>

using namespace sqlite_orm;
using std::cout;
using std::endl;

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
        auto contactIdColumn = make_column("contact_id", &Contact::id, primary_key(), autoincrement());
        {
            using column_type = decltype(contactIdColumn);
            static_assert(internal::is_column<column_type>::value, "");
        }

        auto table = make_table("contacts",
                                contactIdColumn,
                                make_column("first_name", &Contact::firstName),
                                make_column("last_name", &Contact::lastName),
                                make_column("country_code", &Contact::countryCode),
                                make_column("phone_number", &Contact::phoneNumber),
                                make_column("visits_count", &Contact::visitsCount));
        REQUIRE(*table.find_column_name(&Contact::id) == "contact_id");
        REQUIRE(*table.find_column_name(&Contact::firstName) == "first_name");
        REQUIRE(*table.find_column_name(&Contact::lastName) == "last_name");
        REQUIRE(*table.find_column_name(&Contact::countryCode) == "country_code");
        REQUIRE(*table.find_column_name(&Contact::phoneNumber) == "phone_number");
        REQUIRE(*table.find_column_name(&Contact::visitsCount) == "visits_count");
    }
    {
        struct Contact {
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

            const std::string& firstName() const {
                return this->_firstName;
            }

            void setFirstName(std::string value) {
                this->_firstName = move(value);
            }

            const std::string& lastName() const {
                return this->_lastName;
            }

            void setLastName(std::string value) {
                this->_lastName = move(value);
            }

            int countryCode() const {
                return this->_countryCode;
            }

            void setCountryCode(int value) {
                this->_countryCode = value;
            }

            const std::string& phoneNumber() const {
                return this->_phoneNumber;
            }

            void setPhoneNumber(std::string value) {
                this->_phoneNumber = move(value);
            }

            int visitsCount() const {
                return this->_visitsCount;
            }

            void setVisitsCount(int value) {
                this->_visitsCount = value;
            }
        };
        auto table =
            make_table("contacts",
                       make_column("contact_id", &Contact::id, &Contact::setId, primary_key(), autoincrement()),
                       make_column("first_name", &Contact::firstName, &Contact::setFirstName),
                       make_column("last_name", &Contact::lastName, &Contact::setLastName),
                       make_column("country_code", &Contact::countryCode, &Contact::setCountryCode),
                       make_column("phone_number", &Contact::phoneNumber, &Contact::setPhoneNumber),
                       make_column("visits_count", &Contact::visitsCount, &Contact::setVisitsCount));

        REQUIRE(*table.find_column_name(&Contact::id) == "contact_id");
        REQUIRE(*table.find_column_name(&Contact::setId) == "contact_id");

        REQUIRE(*table.find_column_name(&Contact::firstName) == "first_name");
        REQUIRE(*table.find_column_name(&Contact::setFirstName) == "first_name");

        REQUIRE(*table.find_column_name(&Contact::lastName) == "last_name");
        REQUIRE(*table.find_column_name(&Contact::setLastName) == "last_name");

        REQUIRE(*table.find_column_name(&Contact::countryCode) == "country_code");
        REQUIRE(*table.find_column_name(&Contact::setCountryCode) == "country_code");

        REQUIRE(*table.find_column_name(&Contact::phoneNumber) == "phone_number");
        REQUIRE(*table.find_column_name(&Contact::setPhoneNumber) == "phone_number");

        REQUIRE(*table.find_column_name(&Contact::visitsCount) == "visits_count");
        REQUIRE(*table.find_column_name(&Contact::setVisitsCount) == "visits_count");
    }
}

TEST_CASE("Composite key column names") {

    struct User {
        int id = 0;
        std::string name;
        std::string info;
    };

    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info),
                                primary_key(&User::id, &User::name));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        std::vector<std::string> expected = {"id", "name"};
        REQUIRE(std::equal(compositeKeyColumnsNames.begin(), compositeKeyColumnsNames.end(), expected.begin()));
    }
    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info),
                                primary_key(&User::name, &User::id));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        std::vector<std::string> expected = {"name", "id"};
        REQUIRE(std::equal(compositeKeyColumnsNames.begin(), compositeKeyColumnsNames.end(), expected.begin()));
    }
    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info),
                                primary_key(&User::name, &User::id, &User::info));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        std::vector<std::string> expected = {"name", "id", "info"};
        REQUIRE(std::equal(compositeKeyColumnsNames.begin(), compositeKeyColumnsNames.end(), expected.begin()));
    }
    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        REQUIRE(compositeKeyColumnsNames.empty());
    }
}
