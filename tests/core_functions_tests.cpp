#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("substr") {
    struct Test {
        std::string text;
        int x = 0;
        int y = 0;
    };
    auto storage = make_storage(
        {},
        make_table("test", make_column("text", &Test::text), make_column("x", &Test::x), make_column("y", &Test::y)));
    storage.sync_schema();

    {
        auto rows = storage.select(substr("SQLite substr", 8));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "substr");
    }
    {
        storage.insert(Test{"SQLite substr", 8});
        REQUIRE(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "substr");
    }
    {
        auto rows = storage.select(substr("SQLite substr", 1, 6));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "SQLite");
    }
    {

        storage.remove_all<Test>();
        storage.insert(Test{"SQLite substr", 1, 6});
        REQUIRE(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x, &Test::y));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "SQLite");
    }
}

TEST_CASE("zeroblob") {
    struct Test {
        int value = 0;
    };

    auto storage = make_storage({}, make_table("test", make_column("value", &Test::value)));
    storage.sync_schema();

    {
        auto rows = storage.select(zeroblob(10));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(row.size() == 10);
        std::vector<char> expectedValue(10);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        REQUIRE(row == expectedValue);
    }
    {
        storage.insert(Test{100});

        auto rows = storage.select(zeroblob(&Test::value));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(row.size() == 100);
        std::vector<char> expectedValue(100);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        REQUIRE(row == expectedValue);
    }
}

#if SQLITE_VERSION_NUMBER >= 3007016
TEST_CASE("char") {
    auto storage = make_storage({});
    auto rows = storage.select(char_(67, 72, 65, 82));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "CHAR");
}
#endif

TEST_CASE("rtrim") {
    auto storage = make_storage({});
    auto rows = storage.select(rtrim("ototo   "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");

    rows = storage.select(rtrim("ototo   ", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("ltrim") {
    auto storage = make_storage({});
    auto rows = storage.select(ltrim("  ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");

    rows = storage.select(ltrim("  ototo", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("trim") {
    auto storage = make_storage({});
    auto rows = storage.select(trim("   ototo   "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");

    rows = storage.select(trim("   ototo   ", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("upper") {
    auto storage = make_storage({});
    auto rows = storage.select(upper("ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "OTOTO");
}

TEST_CASE("lower") {
    auto storage = make_storage({});
    auto rows = storage.select(lower("OTOTO"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("length") {
    auto storage = make_storage({});
    auto rows = storage.select(length("ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == 5);
}

TEST_CASE("abs") {
    auto storage = make_storage({});
    auto rows = storage.select(sqlite_orm::abs(-10));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front());
    REQUIRE(*rows.front() == 10);
}

TEST_CASE("hex") {
    auto storage = make_storage({});
    {
        auto rows = storage.select(hex(67));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "3637");
    }
    {
        auto rows = storage.select(hex("ä"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "C3A4");
    }
    {
        auto rows = storage.select(hex(nullptr));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == std::string());
    }
}

TEST_CASE("quote") {
    using Catch::Matchers::UnorderedEquals;
    struct Department {
        int id = 0;
        std::string name;
        int managerId = 0;
        int locationId = 0;
    };
    auto storage = make_storage({},
                                make_table("departments",
                                           make_column("department_id", &Department::id, primary_key()),
                                           make_column("department_name", &Department::name),
                                           make_column("manager_id", &Department::managerId),
                                           make_column("location_id", &Department::locationId)));
    storage.sync_schema();
    storage.replace(Department{10, "Administration", 200, 1700});
    storage.replace(Department{20, "Marketing", 201, 1800});
    storage.replace(Department{30, "Purchasing", 114, 1700});
    storage.replace(Department{40, "Human Resources", 203, 2400});
    storage.replace(Department{50, "Shipping", 121, 1500});
    storage.replace(Department{60, "IT", 103, 1400});
    storage.replace(Department{70, "Public Relation", 204, 2700});
    storage.replace(Department{80, "Sales", 145, 2500});
    storage.replace(Department{90, "Executive", 100, 1700});
    storage.replace(Department{100, "Finance", 108, 1700});
    storage.replace(Department{110, "Accounting", 205, 1700});
    storage.replace(Department{120, "Treasury", 0, 1700});
    storage.replace(Department{130, "Corporate Tax", 0, 1700});
    storage.replace(Department{140, "Control And Cre", 0, 1700});
    storage.replace(Department{150, "Shareholder Ser", 0, 1700});
    storage.replace(Department{160, "Benefits", 0, 1700});
    storage.replace(Department{170, "Manufacturing", 0, 1700});
    storage.replace(Department{180, "Construction", 0, 1700});
    storage.replace(Department{190, "Contracting", 0, 1700});
    storage.replace(Department{200, "Operations", 0, 1700});
    storage.replace(Department{210, "IT Support", 0, 1700});
    storage.replace(Department{220, "NOC", 0, 1700});
    storage.replace(Department{230, "IT Helpdesk", 0, 1700});
    storage.replace(Department{240, "Government Sale", 0, 1700});
    storage.replace(Department{250, "Retail Sales", 0, 1700});
    storage.replace(Department{260, "Recruiting", 0, 1700});
    storage.replace(Department{270, "Payroll", 0, 1700});
    {
        auto rows = storage.select(quote("hi"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "'hi'");
    }
    {
        auto rows =
            storage.select(columns(&Department::name, quote(&Department::name)), where(c(&Department::id) > 150));
        std::vector<std::tuple<std::string, std::string>> expected;
        expected.push_back(std::make_tuple("Benefits", "'Benefits'"));
        expected.push_back(std::make_tuple("Manufacturing", "'Manufacturing'"));
        expected.push_back(std::make_tuple("Construction", "'Construction'"));
        expected.push_back(std::make_tuple("Contracting", "'Contracting'"));
        expected.push_back(std::make_tuple("Operations", "'Operations'"));
        expected.push_back(std::make_tuple("IT Support", "'IT Support'"));
        expected.push_back(std::make_tuple("NOC", "'NOC'"));
        expected.push_back(std::make_tuple("IT Helpdesk", "'IT Helpdesk'"));
        expected.push_back(std::make_tuple("Government Sale", "'Government Sale'"));
        expected.push_back(std::make_tuple("Retail Sales", "'Retail Sales'"));
        expected.push_back(std::make_tuple("Recruiting", "'Recruiting'"));
        expected.push_back(std::make_tuple("Payroll", "'Payroll'"));
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
}

TEST_CASE("randomblob") {
    auto storage = make_storage({});
    for(auto i = 0; i < 20; ++i) {
        auto blobLength = i + 1;
        auto rows = storage.select(randomblob(blobLength));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().size() == size_t(blobLength));
    }
}

TEST_CASE("instr") {
    using Catch::Matchers::UnorderedEquals;

    struct Employee {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string address;
    };

    struct sw : alias_tag {
        static const std::string &get() {
            static const std::string res = "sw";
            return res;
        }
    };
    auto storage = make_storage({},
                                make_table("employees",
                                           make_column("id", &Employee::id, primary_key()),
                                           make_column("first_name", &Employee::firstName),
                                           make_column("last_name", &Employee::lastName),
                                           make_column("address", &Employee::address)));
    storage.sync_schema();
    {
        auto rows = storage.select(instr("SQLite Tutorial", "Tutorial"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == 8);
    }
    {
        auto rows = storage.select(instr("SQLite Tutorial", "I"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == 0);
    }
    Employee nancy{1, "Nancy", "Edwards", "825 8 Ave SW"};
    Employee jane{2, "Jane", "Peacock", "1111 6 Ave SW"};
    Employee margaret{3, "Margaret", "Park", "683 10 Street SW"};
    Employee patrick{4, "Patrick", "Jane", "Sacramento Empty House"};
    Employee teresa{5, "Terese", "Lisbon", "Secramento Middle of Nowhere"};
    storage.replace(nancy);
    storage.replace(jane);
    storage.replace(margaret);
    storage.replace(patrick);
    storage.replace(teresa);
    {
        auto rows = storage.select(
            columns(&Employee::lastName, &Employee::firstName, &Employee::address, instr(&Employee::address, "SW")));
        std::vector<std::tuple<std::string, std::string, std::string, int>> expected;
        expected.push_back(std::make_tuple(nancy.lastName, nancy.firstName, nancy.address, 11));
        expected.push_back(std::make_tuple(jane.lastName, jane.firstName, jane.address, 12));
        expected.push_back(std::make_tuple(margaret.lastName, margaret.firstName, margaret.address, 15));
        expected.push_back(std::make_tuple(patrick.lastName, patrick.firstName, patrick.address, 0));
        expected.push_back(std::make_tuple(teresa.lastName, teresa.firstName, teresa.address, 0));
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
    {
        auto rows = storage.select(columns(&Employee::lastName,
                                           &Employee::firstName,
                                           &Employee::address,
                                           as<sw>(instr(&Employee::address, "SW"))),
                                   where(greater_than(get<sw>(), 0)));
        std::vector<std::tuple<std::string, std::string, std::string, int>> expected;
        expected.push_back(std::make_tuple(nancy.lastName, nancy.firstName, nancy.address, 11));
        expected.push_back(std::make_tuple(jane.lastName, jane.firstName, jane.address, 12));
        expected.push_back(std::make_tuple(margaret.lastName, margaret.firstName, margaret.address, 15));
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
}

namespace replace_func_local {
    struct Contact {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string phone;
    };

    bool operator==(const Contact &lhs, const Contact &rhs) {
        return lhs.id == rhs.id && lhs.firstName == rhs.firstName && lhs.lastName == rhs.lastName &&
               lhs.phone == rhs.phone;
    }
}

TEST_CASE("replace func") {
    using Catch::Matchers::UnorderedEquals;
    using namespace replace_func_local;

    auto storage = make_storage({},
                                make_table("contacts",
                                           make_column("contact_id", &Contact::id, primary_key()),
                                           make_column("first_name", &Contact::firstName),
                                           make_column("last_name", &Contact::lastName),
                                           make_column("phone", &Contact::phone)));
    storage.sync_schema();
    {
        auto rows = storage.select(replace("AA B CC AAA", "A", "Z"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "ZZ B CC ZZZ");
    }
    {
        auto rows = storage.select(replace("This is a cat", "This", "That"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "That is a cat");
    }
    Contact john{0, "John", "Doe", "410-555-0168"};
    Contact lily{0, "Lily", "Bush", "410-444-9862"};
    john.id = storage.insert(john);
    lily.id = storage.insert(lily);
    {
        auto contacts = storage.get_all<Contact>();
        std::vector<Contact> expected;
        expected.push_back(john);
        expected.push_back(lily);
        REQUIRE_THAT(contacts, UnorderedEquals(expected));
    }
    storage.update_all(set(c(&Contact::phone) = replace(&Contact::phone, "410", "+1-410")));
    {
        auto contacts = storage.get_all<Contact>();
        john.phone = "+1-410-555-0168";
        lily.phone = "+1-410-444-9862";
        std::vector<Contact> expected;
        expected.push_back(john);
        expected.push_back(lily);
        REQUIRE_THAT(contacts, UnorderedEquals(expected));
    }
}
