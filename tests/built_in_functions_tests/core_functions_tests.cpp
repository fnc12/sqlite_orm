#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("substr") {
    struct Test {
        std::string text;
        int x = 0;
        int y = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Test() = default;
        Test(std::string text, int x, int y) : text{std::move(text)}, x{x}, y{y} {}
#endif
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
        storage.insert(Test{"SQLite substr", 8, 1});
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

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Test() = default;
        Test(int value) : value{value} {}
#endif
    };

    auto storage = make_storage({}, make_table("test", make_column("value", &Test::value)));
    storage.sync_schema();

    {
        auto rows = storage.select(zeroblob(10));
        REQUIRE(rows.size() == 1);
        auto& row = rows.front();
        REQUIRE(row.size() == 10);
        std::vector<char> expectedValue(10);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        REQUIRE(row == expectedValue);
    }
    {
        storage.insert(Test{100});

        auto rows = storage.select(zeroblob(&Test::value));
        REQUIRE(rows.size() == 1);
        auto& row = rows.front();
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

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Department() = default;
        Department(int id, std::string name, int managerId, int locationId) :
            id{id}, name{std::move(name)}, managerId{managerId}, locationId{locationId} {}
#endif
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

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Employee() = default;
        Employee(int id, std::string firstName, std::string lastName, std::string address) :
            id{id}, firstName{std::move(firstName)}, lastName{std::move(lastName)}, address{std::move(address)} {}
#endif
    };

    struct sw : alias_tag {
        static const std::string& get() {
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

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Contact() = default;
        Contact(int id, std::string firstName, std::string lastName, std::string phone) :
            id{id}, firstName{std::move(firstName)}, lastName{std::move(lastName)}, phone{std::move(phone)} {}
#endif
    };

    bool operator==(const Contact& lhs, const Contact& rhs) {
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

TEST_CASE("round") {
    auto storage = make_storage({});
    auto test = [&storage](auto input, double expected) {
        auto rows = storage.select(round(input));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == expected);
    };
    auto test2 = [&storage](auto inputA, auto inputB, double expected) {
        auto rows = storage.select(round(inputA, inputB));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == expected);
    };
    test(23.4, 23.0);
    test(23.6, 24.0);
    test2(23.6985, 2, 23.7);
    test2(190.3985, 3, 190.399);
    test2(99.9, 0, 100.0);
    test2(23.3985, nullptr, 0);  //  maybe this is an error but noone cares AFAIK
    test2(1304.67, -1, 1305.0);
    test2(1929.236, 2, 1929.24);
    test2(1929.236, 1, 1929.2);
    test(1929.236, 1929);
    test(0.5, 1);
    test2(59.9, 0, 60.0);
    test2(-59.9, 0, -60.0);
    test2(-4.535, 2, -4.54);
    test2(34.4158, -1, 34.0);
}

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("coalesce") {
    using Catch::Matchers::Equals;
    using std::nullopt, std::optional, std::vector;

    struct Foo {
        std::optional<double> field;
    };

    auto storage = make_storage({}, make_table("foo", make_column("field", &Foo::field)));
    storage.sync_schema();
    storage.transaction([&storage]() {
        storage.insert<Foo>({});
        storage.insert<Foo>({1.});
        return true;
    });

    SECTION("statement") {
        SECTION("nullptr") {
            auto statement = storage.prepare(select(coalesce<std::optional<double>>(&Foo::field, nullptr)));
            std::ignore = statement;
        }
        SECTION("nullopt") {
            auto statement = storage.prepare(select(coalesce<std::optional<double>>(&Foo::field, std::nullopt)));
            std::ignore = statement;
        }
    }
    SECTION("straight") {
        SECTION("nullptr") {
            storage.select(coalesce<std::optional<double>>(&Foo::field, nullptr));
        }
        SECTION("nullopt") {
            storage.select(coalesce<std::optional<double>>(&Foo::field, std::nullopt));
        }
    }
    SECTION("common return type") {
        auto rows = storage.select(coalesce(&Foo::field, 0), order_by(1));
        REQUIRE_THAT(rows, Equals(vector<optional<double>>{0., 1.}));
    }
}
#endif

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("nullif") {
    using Catch::Matchers::Equals;
    using std::nullopt, std::optional, std::vector;

    struct Foo {
        bool field;
    };

    auto storage = make_storage({}, make_table("foo", make_column("field", &Foo::field)));
    storage.sync_schema();
    storage.transaction([&storage]() {
        storage.insert<Foo>({false});
        storage.insert<Foo>({true});
        return true;
    });

    SECTION("explicit return type") {
        auto rows = storage.select(&Foo::field, where(nullif<std::optional<bool>>(&Foo::field, false)));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows[0] == true);
    }
#if defined(SQLITE_ORM_OPTIONAL_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
    SECTION("common return type") {
        auto rows = storage.select(&Foo::field, where(nullif(&Foo::field, false)));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows[0] == true);
    }
    SECTION("null if 0") {
        auto rows = storage.select(nullif(&Foo::field, 0), order_by(1));
        REQUIRE_THAT(rows, Equals(vector<optional<int>>{nullopt, 1}));
    }
    SECTION("null if 1") {
        auto rows = storage.select(nullif(&Foo::field, 1), order_by(1));
        REQUIRE_THAT(rows, Equals(vector<optional<int>>{nullopt, 0}));
    }
#endif
}
#endif

TEST_CASE("ifnull") {
    //  obtained from here https://www.sqlitetutorial.net/sqlite-functions/sqlite-ifnull/

    using Catch::Matchers::UnorderedEquals;

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

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Customer() = default;
        Customer(int id,
                 std::string firstName,
                 std::string lastName,
                 std::string company,
                 std::string address,
                 std::string city,
                 std::string state,
                 std::string country,
                 std::string postalCode,
                 std::string phone,
                 decltype(fax) fax,
                 std::string email,
                 int supportRepId) :
            id{id},
            firstName{std::move(firstName)}, lastName{std::move(lastName)}, company{std::move(company)},
            address{std::move(address)}, city{std::move(city)}, state{std::move(state)}, country{std::move(country)},
            postalCode{std::move(postalCode)}, phone{std::move(phone)}, fax{std::move(fax)}, email{std::move(email)},
            supportRepId{supportRepId} {}
#endif
    };
    auto storage = make_storage({},
                                make_table("customers",
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
                                           make_column("SupportRepId", &Customer::supportRepId)));
    storage.sync_schema();

    storage.replace(Customer{1,
                             "Luís",
                             "Gonçalves",
                             "Embraer - Empresa Brasileira de Aeronáutica S.A.",
                             "Av. Brigadeiro Faria Lima, 2170",
                             "São José dos Campos",
                             "SP",
                             "Brazil",
                             "12227-000",
                             "+55 (12) 3923-5555",
                             std::make_unique<std::string>("+55 (12) 3923-5566"),
                             "luisg@embraer.com.br",
                             3});
    storage.replace(Customer{2,
                             "Leonie",
                             "Köhler",
                             "",
                             "Theodor-Heuss-Straße 34",
                             "Stuttgart",
                             "",
                             "Germany",
                             "70174",
                             "+49 0711 2842222",
                             nullptr,
                             "leonekohler@surfeu.de",
                             5});
    storage.replace(Customer{3,
                             "François",
                             "Tremblay",
                             "",
                             "1498 rue Bélanger",
                             "Montréal",
                             "QC",
                             "Canada",
                             "H2G 1A7",
                             "+1 (514) 721-4711",
                             nullptr,
                             "ftremblay@gmail.com",
                             3});
    storage.replace(Customer{4,
                             "Bjørn",
                             "Hansen",
                             "",
                             "Ullevålsveien 14",
                             "Oslo",
                             "",
                             "Norway",
                             "0171",
                             "+47 22 44 22 22",
                             nullptr,
                             "bjorn.hansen@yahoo.no",
                             4});
    storage.replace(Customer{5,
                             "František",
                             "Wichterlová",
                             "JetBrains s.r.o.",
                             "Klanova 9/506",
                             "Prague",
                             "",
                             "Czech Republic",
                             "14700",
                             "+420 2 4172 5555",
                             std::make_unique<std::string>("+420 2 4172 5555"),
                             "frantisekw@jetbrains.com",
                             4});
    storage.replace(Customer{6,
                             "Helena",
                             "Holý",
                             "",
                             "Rilská 3174/6",
                             "Prague",
                             "",
                             "Czech Republic",
                             "14300",
                             "+420 2 4177 0449",
                             nullptr,
                             "hholy@gmail.com",
                             5});
    storage.replace(Customer{7,
                             "Astrid",
                             "Gruber",
                             "",
                             "Rotenturmstraße 4, 1010 Innere Stadt",
                             "Vienne",
                             "",
                             "Austria",
                             "1010",
                             "+43 01 5134505",
                             nullptr,
                             "astrid.gruber@apple.at",
                             5});
    storage.replace(Customer{8,
                             "Daan",
                             "Peeters",
                             "",
                             "Grétrystraat 63",
                             "Brussels",
                             "",
                             "Belgium",
                             "1000",
                             "+32 02 219 03 03",
                             nullptr,
                             "daan_peeters@apple.be",
                             4});
    storage.replace(Customer{9,
                             "Kara",
                             "Nielsen",
                             "",
                             "Sønder Boulevard 51",
                             "Copenhagen",
                             "",
                             "Denmark",
                             "1720",
                             "+453 3331 9991",
                             nullptr,
                             "kara.nielsen@jubii.dk",
                             4});
    storage.replace(Customer{10,
                             "Eduardo",
                             "Martins",
                             "Woodstock Discos",
                             "Rua Dr. Falcão Filho, 155",
                             "São Paulo",
                             "SP",
                             "Brazil",
                             "01007-010",
                             "+55 (11) 3033-5446",
                             std::make_unique<std::string>("+55 (11) 3033-4564"),
                             "eduardo@woodstock.com.br",
                             4});
    storage.replace(Customer{11,
                             "Alexandre",
                             "Rocha",
                             "Banco do Brasil S.A.",
                             "Av. Paulista, 2022",
                             "São Paulo",
                             "SP",
                             "Brazil",
                             "01310-200",
                             "+55 (11) 3055-3278",
                             std::make_unique<std::string>("+55 (11) 3055-8131"),
                             "alero@uol.com.br",
                             5});
    storage.replace(Customer{12,
                             "Roberto",
                             "Almeida",
                             "Riotur",
                             "Praça Pio X, 119",
                             "Rio de Janeiro",
                             "RJ",
                             "Brazil",
                             "20040-020",
                             "+55 (21) 2271-7000",
                             std::make_unique<std::string>("+55 (21) 2271-7070"),
                             "roberto.almeida@riotur.gov.br",
                             3});
    storage.replace(Customer{13,
                             "Fernanda",
                             "Ramos",
                             "",
                             "Qe 7 Bloco G",
                             "Brasília",
                             "DF",
                             "Brazil",
                             "71020-677",
                             "+55 (61) 3363-5547",
                             std::make_unique<std::string>("+55 (61) 3363-7855"),
                             "fernadaramos4@uol.com.br",
                             4});
    storage.replace(Customer{14,
                             "Mark",
                             "Philips",
                             "Telus",
                             "8210 111 ST NW",
                             "Edmonton",
                             "AB",
                             "Canada",
                             "T6G 2C7",
                             "+1 (780) 434-4554",
                             std::make_unique<std::string>("+1 (780) 434-5565"),
                             "mphilips12@shaw.ca",
                             5});
    storage.replace(Customer{15,
                             "Jennifer",
                             "Peterson",
                             "Rogers Canada",
                             "700 W Pender Street",
                             "Vancouver",
                             "BC",
                             "Canada",
                             "V6C 1G8",
                             "+1 (604) 688-2255",
                             std::make_unique<std::string>("+1 (604) 688-8756"),
                             "jenniferp@rogers.ca",
                             3});
    storage.replace(Customer{16,
                             "Frank",
                             "Harris",
                             "Google Inc.",
                             "1600 Amphitheatre Parkway",
                             "Mountain View",
                             "CA",
                             "USA",
                             "94043-1351",
                             "+1 (650) 253-0000",
                             std::make_unique<std::string>("+1 (650) 253-0000"),
                             "fharris@google.com",
                             4});
    storage.replace(Customer{17,
                             "Jack",
                             "Smith",
                             "Microsoft Corporation",
                             "1 Microsoft Way",
                             "Redmond",
                             "WA",
                             "USA",
                             "98052-8300",
                             "+1 (425) 882-8080",
                             std::make_unique<std::string>("+1 (425) 882-8081"),
                             "jacksmith@microsoft.com",
                             5});
    storage.replace(Customer{18,
                             "Michelle",
                             "Brooks",
                             "",
                             "627 Broadway",
                             "New York",
                             "NY",
                             "USA",
                             "10012-2612",
                             "+1 (212) 221-3546",
                             std::make_unique<std::string>("+1 (212) 221-4679"),
                             "michelleb@aol.com",
                             3});
    storage.replace(Customer{19,
                             "Tim",
                             "Goyer",
                             "Apple Inc.",
                             "1 Infinite Loop",
                             "Cupertino",
                             "CA",
                             "USA",
                             "95014",
                             "+1 (408) 996-1010",
                             std::make_unique<std::string>("+1 (408) 996-1011"),
                             "tgoyer@apple.com",
                             3});
    storage.replace(Customer{20,
                             "Dan",
                             "Miller",
                             "",
                             "541 Del Medio Avenue",
                             "Mountain View",
                             "CA",
                             "USA",
                             "94040-111",
                             "+1 (650) 644-3358",
                             nullptr,
                             "dmiller@comcast.com",
                             4});
    storage.replace(Customer{21,
                             "Kathy",
                             "Chase",
                             "",
                             "801 W 4th Street",
                             "Reno",
                             "NV",
                             "USA",
                             "89503",
                             "+1 (775) 223-7665",
                             nullptr,
                             "kachase@hotmail.com",
                             5});
    storage.replace(Customer{22,
                             "Heather",
                             "Leacock",
                             "",
                             "120 S Orange Ave",
                             "Orlando",
                             "FL",
                             "USA",
                             "32801",
                             "+1 (407) 999-7788",
                             nullptr,
                             "hleacock@gmail.com",
                             4});
    storage.replace(Customer{23,
                             "John",
                             "Gordon",
                             "",
                             "69 Salem Street",
                             "Boston",
                             "MA",
                             "USA",
                             "2113",
                             "+1 (617) 522-1333",
                             nullptr,
                             "johngordon22@yahoo.com",
                             4});
    storage.replace(Customer{24,
                             "Frank",
                             "Ralston",
                             "",
                             "162 E Superior Street",
                             "Chicago",
                             "IL",
                             "USA",
                             "60611",
                             "+1 (312) 332-3232",
                             nullptr,
                             "fralston@gmail.com",
                             3});
    storage.replace(Customer{25,
                             "Victor",
                             "Stevens",
                             "",
                             "319 N. Frances Street",
                             "Madison",
                             "WI",
                             "USA",
                             "53703",
                             "+1 (608) 257-0597",
                             nullptr,
                             "vstevens@yahoo.com",
                             5});
    storage.replace(Customer{26,
                             "Richard",
                             "Cunningham",
                             "",
                             "2211 W Berry Street",
                             "Fort Worth",
                             "TX",
                             "USA",
                             "76110",
                             "+1 (817) 924-7272",
                             nullptr,
                             "ricunningham@hotmail.com",
                             4});
    storage.replace(Customer{27,
                             "Patrick",
                             "Gray",
                             "",
                             "1033 N Park Ave",
                             "Tucson",
                             "AZ",
                             "USA",
                             "85719",
                             "+1 (520) 622-4200",
                             nullptr,
                             "patrick.gray@aol.com",
                             4});
    storage.replace(Customer{28,
                             "Julia",
                             "Barnett",
                             "",
                             "302 S 700 E",
                             "Salt Lake City",
                             "UT",
                             "USA",
                             "84102",
                             "+1 (801) 531-7272",
                             nullptr,
                             "jubarnett@gmail.com",
                             5});
    storage.replace(Customer{29,
                             "Robert",
                             "Brown",
                             "",
                             "796 Dundas Street West",
                             "Toronto",
                             "ON",
                             "Canada",
                             "M6J 1V1",
                             "+1 (416) 363-8888",
                             nullptr,
                             "robbrown@shaw.ca",
                             3});
    storage.replace(Customer{30,
                             "Edward",
                             "Francis",
                             "",
                             "230 Elgin Street",
                             "Ottawa",
                             "ON",
                             "Canada",
                             "K2P 1L7",
                             "+1 (613) 234-3322",
                             nullptr,
                             "edfrancis@yachoo.ca",
                             3});
    storage.replace(Customer{31,
                             "Martha",
                             "Silk",
                             "",
                             "194A Chain Lake Drive",
                             "Halifax",
                             "NS",
                             "Canada",
                             "B3S 1C5",
                             "+1 (902) 450-0450",
                             nullptr,
                             "marthasilk@gmail.com",
                             5});
    storage.replace(Customer{32,
                             "Aaron",
                             "Mitchell",
                             "",
                             "696 Osborne Street",
                             "Winnipeg",
                             "MB",
                             "Canada",
                             "R3L 2B9",
                             "+1 (204) 452-6452",
                             nullptr,
                             "aaronmitchell@yahoo.ca",
                             4});
    storage.replace(Customer{33,
                             "Ellie",
                             "Sullivan",
                             "",
                             "5112 48 Street",
                             "Yellowknife",
                             "NT",
                             "Canada",
                             "X1A 1N6",
                             "+1 (867) 920-2233",
                             nullptr,
                             "ellie.sullivan@shaw.ca",
                             3});
    storage.replace(Customer{34,
                             "João",
                             "Fernandes",
                             "",
                             "Rua da Assunção 53",
                             "Lisbon",
                             "",
                             "Portugal",
                             "",
                             "+351 (213) 466-111",
                             nullptr,
                             "jfernandes@yahoo.pt",
                             4});
    storage.replace(Customer{35,
                             "Madalena",
                             "Sampaio",
                             "",
                             "Rua dos Campeões Europeus de Viena, 4350",
                             "Porto",
                             "",
                             "Portugal",
                             "",
                             "+351 (225) 022-448",
                             nullptr,
                             "masampaio@sapo.pt",
                             4});
    storage.replace(Customer{36,
                             "Hannah",
                             "Schneider",
                             "",
                             "Tauentzienstraße 8",
                             "Berlin",
                             "",
                             "Germany",
                             "10789",
                             "+49 030 26550280",
                             nullptr,
                             "hannah.schneider@yahoo.de",
                             5});
    storage.replace(Customer{37,
                             "Fynn",
                             "Zimmermann",
                             "",
                             "Berger Straße 10",
                             "Frankfurt",
                             "",
                             "Germany",
                             "60316",
                             "+49 069 40598889",
                             nullptr,
                             "fzimmermann@yahoo.de",
                             3});
    storage.replace(Customer{38,
                             "Niklas",
                             "Schröder",
                             "",
                             "Barbarossastraße 19",
                             "Berlin",
                             "",
                             "Germany",
                             "10779",
                             "+49 030 2141444",
                             nullptr,
                             "nschroder@surfeu.de",
                             3});
    storage.replace(Customer{39,
                             "Camille",
                             "Bernard",
                             "",
                             "4, Rue Milton",
                             "Paris",
                             "",
                             "France",
                             "75009",
                             "+33 01 49 70 65 65",
                             nullptr,
                             "camille.bernard@yahoo.fr",
                             4});
    storage.replace(Customer{40,
                             "Dominique",
                             "Lefebvre",
                             "",
                             "8, Rue Hanovre",
                             "Paris",
                             "",
                             "France",
                             "75002",
                             "+33 01 47 42 71 71",
                             nullptr,
                             "dominiquelefebvre@gmail.com",
                             4});
    storage.replace(Customer{41,
                             "Marc",
                             "Dubois",
                             "",
                             "11, Place Bellecour",
                             "Lyon",
                             "",
                             "France",
                             "69002",
                             "+33 04 78 30 30 30",
                             nullptr,
                             "marc.dubois@hotmail.com",
                             5});
    storage.replace(Customer{42,
                             "Wyatt",
                             "Girard",
                             "",
                             "9, Place Louis Barthou",
                             "Bordeaux",
                             "",
                             "France",
                             "33000",
                             "+33 05 56 96 96 96",
                             nullptr,
                             "wyatt.girard@yahoo.fr",
                             3});
    storage.replace(Customer{43,
                             "Isabelle",
                             "Mercier",
                             "",
                             "68, Rue Jouvence",
                             "Dijon",
                             "",
                             "France",
                             "21000",
                             "+33 03 80 73 66 99",
                             nullptr,
                             "isabelle_mercier@apple.fr",
                             3});
    storage.replace(Customer{44,
                             "Terhi",
                             "Hämäläinen",
                             "",
                             "Porthaninkatu 9",
                             "Helsinki",
                             "",
                             "Finland",
                             "00530",
                             "+358 09 870 2000",
                             nullptr,
                             "terhi.hamalainen@apple.fi",
                             3});
    storage.replace(Customer{45,
                             "Ladislav",
                             "Kovács",
                             "",
                             "Erzsébet krt. 58.",
                             "Budapest",
                             "",
                             "Hungary",
                             "H-1073",
                             "",
                             nullptr,
                             "ladislav_kovacs@apple.hu",
                             3});
    storage.replace(Customer{46,
                             "Hugh",
                             "O'Reilly",
                             "",
                             "3 Chatham Street",
                             "Dublin",
                             "Dublin",
                             "Ireland",
                             "",
                             "+353 01 6792424",
                             nullptr,
                             "hughoreilly@apple.ie",
                             3});
    storage.replace(Customer{47,
                             "Lucas",
                             "Mancini",
                             "",
                             "Via Degli Scipioni, 43",
                             "Rome",
                             "RM",
                             "Italy",
                             "00192",
                             "+39 06 39733434",
                             nullptr,
                             "lucas.mancini@yahoo.it",
                             5});
    storage.replace(Customer{48,
                             "Johannes",
                             "Van der Berg",
                             "",
                             "Lijnbaansgracht 120bg",
                             "Amsterdam",
                             "VV",
                             "Netherlands",
                             "1016",
                             "+31 020 6223130",
                             nullptr,
                             "johavanderberg@yahoo.nl",
                             5});
    storage.replace(Customer{49,
                             "Stanisław",
                             "Wójcik",
                             "",
                             "Ordynacka 10",
                             "Warsaw",
                             "",
                             "Poland",
                             "00-358",
                             "+48 22 828 37 39",
                             nullptr,
                             "stanisław.wójcik@wp.pl",
                             4});
    storage.replace(Customer{50,
                             "Enrique",
                             "Muñoz",
                             "",
                             "C/ San Bernardo 85",
                             "Madrid",
                             "",
                             "Spain",
                             "28015",
                             "+34 914 454 454",
                             nullptr,
                             "enrique_munoz@yahoo.es",
                             5});
    storage.replace(Customer{51,
                             "Joakim",
                             "Johansson",
                             "",
                             "Celsiusg. 9",
                             "Stockholm",
                             "",
                             "Sweden",
                             "11230",
                             "+46 08-651 52 52",
                             nullptr,
                             "joakim.johansson@yahoo.se",
                             5});
    storage.replace(Customer{52,
                             "Emma",
                             "Jones",
                             "",
                             "202 Hoxton Street",
                             "London",
                             "",
                             "United Kingdom",
                             "N1 5LH",
                             "+44 020 7707 0707",
                             nullptr,
                             "emma_jones@hotmail.com",
                             3});
    storage.replace(Customer{53,
                             "Phil",
                             "Hughes",
                             "",
                             "113 Lupus St",
                             "London",
                             "",
                             "United Kingdom",
                             "SW1V 3EN",
                             "+44 020 7976 5722",
                             nullptr,
                             "phil.hughes@gmail.com",
                             3});
    storage.replace(Customer{54,
                             "Steve",
                             "Murray",
                             "",
                             "110 Raeburn Pl",
                             "Edinburgh ",
                             "",
                             "United Kingdom",
                             "EH4 1HH",
                             "+44 0131 315 3300",
                             nullptr,
                             "steve.murray@yahoo.uk",
                             5});
    storage.replace(Customer{55,
                             "Mark",
                             "Taylor",
                             "",
                             "421 Bourke Street",
                             "Sidney",
                             "NSW",
                             "Australia",
                             "2010",
                             "+61 (02) 9332 3633",
                             nullptr,
                             "mark.taylor@yahoo.au",
                             4});
    storage.replace(Customer{56,
                             "Diego",
                             "Gutiérrez",
                             "",
                             "307 Macacha Güemes",
                             "Buenos Aires",
                             "",
                             "Argentina",
                             "1106",
                             "+54 (0)11 4311 4333",
                             nullptr,
                             "diego.gutierrez@yahoo.ar",
                             4});
    storage.replace(Customer{57,
                             "Luis",
                             "Rojas",
                             "",
                             "Calle Lira, 198",
                             "Santiago",
                             "",
                             "Chile",
                             "",
                             "+56 (0)2 635 4444",
                             nullptr,
                             "luisrojas@yahoo.cl",
                             5});
    storage.replace(Customer{58,
                             "Manoj",
                             "Pareek",
                             "",
                             "12,Community Centre",
                             "Delhi",
                             "",
                             "India",
                             "110017",
                             "+91 0124 39883988",
                             nullptr,
                             "manoj.pareek@rediff.com",
                             3});
    storage.replace(Customer{59,
                             "Puja",
                             "Srivastava",
                             "",
                             "3,Raj Bhavan Road",
                             "Bangalore",
                             "",
                             "India",
                             "560001",
                             "+91 080 22289999",
                             nullptr,
                             "puja_srivastava@yahoo.in",
                             3});

    auto rows = storage.select(columns(&Customer::firstName,
                                       &Customer::lastName,
                                       ifnull<std::string>(&Customer::fax, "Call:" || c(&Customer::phone))),
                               order_by(&Customer::firstName));
    decltype(rows) expected;
    expected.reserve(rows.size());
    expected.push_back({"Aaron", "Mitchell", "Call:+1 (204) 452-6452"});
    expected.push_back({"Alexandre", "Rocha", "+55 (11) 3055-8131"});
    expected.push_back({"Astrid", "Gruber", "Call:+43 01 5134505"});
    expected.push_back({"Bjørn", "Hansen", "Call:+47 22 44 22 22"});
    expected.push_back({"Camille", "Bernard", "Call:+33 01 49 70 65 65"});
    expected.push_back({"Daan", "Peeters", "Call:+32 02 219 03 03"});
    expected.push_back({"Dan", "Miller", "Call:+1 (650) 644-3358"});
    expected.push_back({"Diego", "Gutiérrez", "Call:+54 (0)11 4311 4333"});
    expected.push_back({"Dominique", "Lefebvre", "Call:+33 01 47 42 71 71"});
    expected.push_back({"Eduardo", "Martins", "+55 (11) 3033-4564"});
    expected.push_back({"Edward", "Francis", "Call:+1 (613) 234-3322"});
    expected.push_back({"Ellie", "Sullivan", "Call:+1 (867) 920-2233"});
    expected.push_back({"Emma", "Jones", "Call:+44 020 7707 0707"});
    expected.push_back({"Enrique", "Muñoz", "Call:+34 914 454 454"});
    expected.push_back({"Fernanda", "Ramos", "+55 (61) 3363-7855"});
    expected.push_back({"Frank", "Harris", "+1 (650) 253-0000"});
    expected.push_back({"Frank", "Ralston", "Call:+1 (312) 332-3232"});
    expected.push_back({"František", "Wichterlová", "+420 2 4172 5555"});
    expected.push_back({"François", "Tremblay", "Call:+1 (514) 721-4711"});
    expected.push_back({"Fynn", "Zimmermann", "Call:+49 069 40598889"});
    expected.push_back({"Hannah", "Schneider", "Call:+49 030 26550280"});
    expected.push_back({"Heather", "Leacock", "Call:+1 (407) 999-7788"});
    expected.push_back({"Helena", "Holý", "Call:+420 2 4177 0449"});
    expected.push_back({"Hugh", "O'Reilly", "Call:+353 01 6792424"});
    expected.push_back({"Isabelle", "Mercier", "Call:+33 03 80 73 66 99"});
    expected.push_back({"Jack", "Smith", "+1 (425) 882-8081"});
    expected.push_back({"Jennifer", "Peterson", "+1 (604) 688-8756"});
    expected.push_back({"Joakim", "Johansson", "Call:+46 08-651 52 52"});
    expected.push_back({"Johannes", "Van der Berg", "Call:+31 020 6223130"});
    expected.push_back({"John", "Gordon", "Call:+1 (617) 522-1333"});
    expected.push_back({"João", "Fernandes", "Call:+351 (213) 466-111"});
    expected.push_back({"Julia", "Barnett", "Call:+1 (801) 531-7272"});
    expected.push_back({"Kara", "Nielsen", "Call:+453 3331 9991"});
    expected.push_back({"Kathy", "Chase", "Call:+1 (775) 223-7665"});
    expected.push_back({"Ladislav", "Kovács", "Call:"});
    expected.push_back({"Leonie", "Köhler", "Call:+49 0711 2842222"});
    expected.push_back({"Lucas", "Mancini", "Call:+39 06 39733434"});
    expected.push_back({"Luis", "Rojas", "Call:+56 (0)2 635 4444"});
    expected.push_back({"Luís", "Gonçalves", "+55 (12) 3923-5566"});
    expected.push_back({"Madalena", "Sampaio", "Call:+351 (225) 022-448"});
    expected.push_back({"Manoj", "Pareek", "Call:+91 0124 39883988"});
    expected.push_back({"Marc", "Dubois", "Call:+33 04 78 30 30 30"});
    expected.push_back({"Mark", "Philips", "+1 (780) 434-5565"});
    expected.push_back({"Mark", "Taylor", "Call:+61 (02) 9332 3633"});
    expected.push_back({"Martha", "Silk", "Call:+1 (902) 450-0450"});
    expected.push_back({"Michelle", "Brooks", "+1 (212) 221-4679"});
    expected.push_back({"Niklas", "Schröder", "Call:+49 030 2141444"});
    expected.push_back({"Patrick", "Gray", "Call:+1 (520) 622-4200"});
    expected.push_back({"Phil", "Hughes", "Call:+44 020 7976 5722"});
    expected.push_back({"Puja", "Srivastava", "Call:+91 080 22289999"});
    expected.push_back({"Richard", "Cunningham", "Call:+1 (817) 924-7272"});
    expected.push_back({"Robert", "Brown", "Call:+1 (416) 363-8888"});
    expected.push_back({"Roberto", "Almeida", "+55 (21) 2271-7070"});
    expected.push_back({"Stanisław", "Wójcik", "Call:+48 22 828 37 39"});
    expected.push_back({"Steve", "Murray", "Call:+44 0131 315 3300"});
    expected.push_back({"Terhi", "Hämäläinen", "Call:+358 09 870 2000"});
    expected.push_back({"Tim", "Goyer", "+1 (408) 996-1011"});
    expected.push_back({"Victor", "Stevens", "Call:+1 (608) 257-0597"});
    expected.push_back({"Wyatt", "Girard", "Call:+33 05 56 96 96 96"});
    REQUIRE_THAT(rows, UnorderedEquals(expected));
}
