#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("table::find_column_name") {
    SECTION("fields") {
        struct Contact {
            int id = 0;
            std::string firstName;
            std::string lastName;
            int countryCode = 0;
            std::string phoneNumber;
            int visitsCount = 0;
        };
        auto contactIdColumn = make_column("contact_id", &Contact::id, primary_key().autoincrement());
        {
            using column_type = decltype(contactIdColumn);
            STATIC_REQUIRE(internal::is_column<column_type>::value);
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
    SECTION("getters and setters") {
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
                this->_firstName = std::move(value);
            }

            const std::string& lastName() const {
                return this->_lastName;
            }

            void setLastName(std::string value) {
                this->_lastName = std::move(value);
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
                this->_phoneNumber = std::move(value);
            }

            int visitsCount() const {
                return this->_visitsCount;
            }

            void setVisitsCount(int value) {
                this->_visitsCount = value;
            }
        };
        auto table = make_table("contacts",
                                make_column("contact_id", &Contact::id, &Contact::setId, primary_key().autoincrement()),
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

TEST_CASE("for_each_foreign_key") {
    struct Location {
        int id;
        std::string place;
        std::string country;
        std::string city;
        int distance;
    };

    struct Visit {
        int id;
        std::unique_ptr<int> location;
        std::unique_ptr<int> user;
        int visited_at;
        uint8_t mark;
    };
    auto locationTable = make_table("location",
                                    make_column("id", &Location::id, primary_key()),
                                    make_column("place", &Location::place),
                                    make_column("country", &Location::country),
                                    make_column("city", &Location::city),
                                    make_column("distance", &Location::distance));
    auto visitTable = make_table("visit",
                                 make_column("id", &Visit::id, primary_key()),
                                 make_column("location", &Visit::location),
                                 make_column("user", &Visit::user),
                                 make_column("visited_at", &Visit::visited_at),
                                 make_column("mark", &Visit::mark),
                                 foreign_key(&Visit::location).references(&Location::id));
    locationTable.for_each_foreign_key([](auto&) {
        REQUIRE(false);
    });
    auto visitCallsCount = 0;
    visitTable.for_each_foreign_key([&visitCallsCount](auto& foreignKey) {
        ++visitCallsCount;
        REQUIRE(foreignKey == foreign_key(&Visit::location).references(&Location::id));
    });
    REQUIRE(visitCallsCount == 1);
}
