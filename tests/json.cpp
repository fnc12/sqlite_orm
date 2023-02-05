#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

#ifdef SQLITE_ENABLE_JSON1
TEST_CASE("json") {
    auto storage = make_storage("");
    std::vector<std::string> expected;
    std::vector<std::string> rows;
    rows = storage.select(json(" { \"this\" : \"is\", \"a\": [ \"test\" ] } "));
    expected.push_back("{\"this\":\"is\",\"a\":[\"test\"]}");
    REQUIRE(expected == rows);
}

TEST_CASE("json_array") {
    auto storage = make_storage("");
    std::vector<std::string> expected;
    std::vector<std::string> rows;
    SECTION("1") {
        rows = storage.select(json_array(1, 2, "3", 4));
        expected.push_back("[1,2,\"3\",4]");
    }
    SECTION("2") {
        rows = storage.select(json_array("[1,2]"));
        expected.push_back("[\"[1,2]\"]");
    }
    SECTION("3") {
        rows = storage.select(json_array(json_array(1, 2)));
        expected.push_back("[[1,2]]");
    }
    SECTION("4") {
        rows = storage.select(json_array(1, nullptr, "3", "[4,5]", "{\"six\":7.7}"));
        expected.push_back(R"([1,null,"3","[4,5]","{\"six\":7.7}"])");
    }
    SECTION("5") {
        rows = storage.select(json_array(1, nullptr, "3", json("[4,5]"), json("{\"six\":7.7}")));
        expected.push_back(R"([1,null,"3",[4,5],{"six":7.7}])");
    }
    REQUIRE(expected == rows);
}

TEST_CASE("json_array_length") {
    auto storage = make_storage("");
    std::vector<int> expected;
    std::vector<int> rows;
    SECTION("1") {
        rows = storage.select(json_array_length("[1,2,3,4]"));
        expected.push_back(4);
    }
    SECTION("2") {
        rows = storage.select(json_array_length("[1,2,3,4]", "$"));
        expected.push_back(4);
    }
    SECTION("3") {
        rows = storage.select(json_array_length("[1,2,3,4]", "$[2]"));
        expected.push_back(0);
    }
    SECTION("4") {
        rows = storage.select(json_array_length("{\"one\":[1,2,3]}"));
        expected.push_back(0);
    }
    SECTION("5") {
        rows = storage.select(json_array_length("{\"one\":[1,2,3]}", "$.one"));
        expected.push_back(3);
    }
    REQUIRE(expected == rows);
}

TEST_CASE("json_array_length nullable") {
    auto storage = make_storage("");
    using Type = std::unique_ptr<int>;
    Type expected;
    Type value;
    std::vector<Type> rows;
    SECTION("1") {
        rows = storage.select(json_array_length<Type>("[1,2,3,4]"));
        expected = std::make_unique<int>(4);
    }
    SECTION("2") {
        rows = storage.select(json_array_length<Type>("[1,2,3,4]", "$"));
        expected = std::make_unique<int>(4);
    }
    SECTION("3") {
        rows = storage.select(json_array_length<Type>("[1,2,3,4]", "$[2]"));
        expected = std::make_unique<int>(0);
    }
    SECTION("4") {
        rows = storage.select(json_array_length<Type>("{\"one\":[1,2,3]}"));
        expected = std::make_unique<int>(0);
    }
    SECTION("5") {
        rows = storage.select(json_array_length<Type>("{\"one\":[1,2,3]}", "$.one"));
        expected = std::make_unique<int>(3);
    }
    SECTION("6") {
        rows = storage.select(json_array_length<Type>("{\"one\":[1,2,3]}", "$.two"));
        expected = nullptr;
    }
    value = std::move(rows[0]);
    REQUIRE(rows.size() == 1);
    REQUIRE(bool(expected) == bool(value));
    if(expected) {
        REQUIRE(*expected == *value);
    }
}

TEST_CASE("json_extract") {
    auto storage = make_storage("");
    SECTION("1") {
        auto rows = storage.select(json_extract<std::string>("{\"a\":2,\"c\":[4,5,{\"f\":7}]}", "$"));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":[4,5,{"f":7}]})");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_extract<std::string>("{\"a\":2,\"c\":[4,5,{\"f\":7}]}", "$.c"));
        decltype(rows) expected;
        expected.push_back(R"([4,5,{"f":7}])");
        REQUIRE(expected == rows);
    }
    SECTION("3") {
        auto rows = storage.select(json_extract<std::string>("{\"a\":2,\"c\":[4,5,{\"f\":7}]}", "$.c[2]"));
        decltype(rows) expected;
        expected.push_back(R"({"f":7})");
        REQUIRE(expected == rows);
    }
    SECTION("4") {
        auto rows = storage.select(json_extract<int>("{\"a\":2,\"c\":[4,5,{\"f\":7}]}", "$.c[2].f"));
        decltype(rows) expected;
        expected.push_back(7);
        REQUIRE(expected == rows);
    }
    SECTION("5") {
        auto rows = storage.select(json_extract<std::string>(R"({"a":2,"c":[4,5],"f":7})", "$.c", "$.a"));
        decltype(rows) expected;
        expected.push_back("[[4,5],2]");
        REQUIRE(expected == rows);
    }
    SECTION("6") {
        auto rows = storage.select(json_extract<int>(R"({"a":2,"c":[4,5],"f":7})", "$.c[#-1]"));
        decltype(rows) expected;
        expected.push_back(5);
        REQUIRE(expected == rows);
    }
    SECTION("7") {
        auto rows = storage.select(json_extract<std::unique_ptr<std::string>>(R"({"a":2,"c":[4,5,{"f":7}]})", "$.x"));
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows[0]);
    }
    SECTION("8") {
        auto rows = storage.select(json_extract<std::string>(R"({"a":2,"c":[4,5,{"f":7}]})", "$.x", "$.a"));
        decltype(rows) expected;
        expected.push_back("[null,2]");
        REQUIRE(expected == rows);
    }
}

TEST_CASE("json_insert") {
    auto storage = make_storage("");
    SECTION("1") {
        auto rows = storage.select(json_insert("[1,2,3,4]", "$[#]", 99));
        decltype(rows) expected;
        expected.push_back("[1,2,3,4,99]");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_insert("[1,[2,3],4]", "$[1][#]", 99));
        decltype(rows) expected;
        expected.push_back("[1,[2,3,99],4]");
        REQUIRE(expected == rows);
    }
    SECTION("3") {
        auto rows = storage.select(json_insert(R"({"a":2,"c":4})", "$.a", 99));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":4})");
        REQUIRE(expected == rows);
    }
    SECTION("4") {
        auto rows = storage.select(json_insert(R"({"a":2,"c":4})", "$.e", 99));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":4,"e":99})");
        REQUIRE(expected == rows);
    }
}

TEST_CASE("json_replace") {
    auto storage = make_storage("");
    SECTION("1") {
        auto rows = storage.select(json_replace(R"({"a":2,"c":4})", "$.a", 99));
        decltype(rows) expected;
        expected.push_back(R"({"a":99,"c":4})");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_replace(R"({"a":2,"c":4})", "$.e", 99));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":4})");
        REQUIRE(expected == rows);
    }
}

TEST_CASE("json_set") {
    auto storage = make_storage("");
    SECTION("1") {
        auto rows = storage.select(json_set(R"({"a":2,"c":4})", "$.a", 99));
        decltype(rows) expected;
        expected.push_back(R"({"a":99,"c":4})");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_set(R"({"a":2,"c":4})", "$.e", 99));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":4,"e":99})");
        REQUIRE(expected == rows);
    }
    SECTION("3") {
        auto rows = storage.select(json_set(R"({"a":2,"c":4})", "$.c", "[97,96]"));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":"[97,96]"})");
        REQUIRE(expected == rows);
    }
    SECTION("4") {
        auto rows = storage.select(json_set(R"({"a":2,"c":4})", "$.c", json("[97,96]")));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":[97,96]})");
        REQUIRE(expected == rows);
    }
    SECTION("5") {
        auto rows = storage.select(json_set(R"({"a":2,"c":4})", "$.c", json_array(97, 96)));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":[97,96]})");
        REQUIRE(expected == rows);
    }
}

TEST_CASE("json_object") {
    auto storage = make_storage("");
    SECTION("1") {
        auto rows = storage.select(json_object("a", 2, "c", 4));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":4})");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_object("a", 2, "c", "{e:5}"));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":"{e:5}"})");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_object("a", 2, "c", json_object("e", 5)));
        decltype(rows) expected;
        expected.push_back(R"({"a":2,"c":{"e":5}})");
        REQUIRE(expected == rows);
    }
}

TEST_CASE("json_patch") {
    auto storage = make_storage("");
    SECTION("1") {
        auto rows = storage.select(json_patch(R"({"a":1,"b":2})", R"({"c":3,"d":4})"));
        decltype(rows) expected;
        expected.push_back(R"({"a":1,"b":2,"c":3,"d":4})");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_patch(R"({"a":[1,2],"b":2})", R"({"a":9})"));
        decltype(rows) expected;
        expected.push_back(R"({"a":9,"b":2})");
        REQUIRE(expected == rows);
    }
    SECTION("3") {
        auto rows = storage.select(json_patch(R"({"a":[1,2],"b":2})", R"({"a":null})"));
        decltype(rows) expected;
        expected.push_back(R"({"b":2})");
        REQUIRE(expected == rows);
    }
    SECTION("4") {
        auto rows = storage.select(json_patch(R"({"a":1,"b":2})", R"({"a":9,"b":null,"c":8})"));
        decltype(rows) expected;
        expected.push_back(R"({"a":9,"c":8})");
        REQUIRE(expected == rows);
    }
    SECTION("5") {
        auto rows = storage.select(json_patch(R"({"a":{"x":1,"y":2},"b":3})", R"({"a":{"y":9},"c":8})"));
        decltype(rows) expected;
        expected.push_back(R"({"a":{"x":1,"y":9},"b":3,"c":8})");
        REQUIRE(expected == rows);
    }
}

TEST_CASE("json_remove") {
    auto storage = make_storage("");
    SECTION("1") {
        auto rows = storage.select(json_remove(R"([0,1,2,3,4])", "$[2]"));
        decltype(rows) expected;
        expected.push_back(R"([0,1,3,4])");
        REQUIRE(expected == rows);
    }
    SECTION("2") {
        auto rows = storage.select(json_remove(R"([0,1,2,3,4])", "$[2]", "$[0]"));
        decltype(rows) expected;
        expected.push_back(R"([1,3,4])");
        REQUIRE(expected == rows);
    }
    SECTION("3") {
        auto rows = storage.select(json_remove(R"([0,1,2,3,4])", "$[0]", "$[2]"));
        decltype(rows) expected;
        expected.push_back(R"([1,2,4])");
        REQUIRE(expected == rows);
    }
    SECTION("4") {
        auto rows = storage.select(json_remove(R"([0,1,2,3,4])", "$[#-1]", "$[0]"));
        decltype(rows) expected;
        expected.push_back(R"([1,2,3])");
        REQUIRE(expected == rows);
    }
    SECTION("5") {
        auto rows = storage.select(json_remove(R"({"x":25,"y":42})"));
        decltype(rows) expected;
        expected.push_back(R"({"x":25,"y":42})");
        REQUIRE(expected == rows);
    }
    SECTION("6") {
        auto rows = storage.select(json_remove(R"({"x":25,"y":42})", "$.z"));
        decltype(rows) expected;
        expected.push_back(R"({"x":25,"y":42})");
        REQUIRE(expected == rows);
    }
    SECTION("7") {
        auto rows = storage.select(json_remove(R"({"x":25,"y":42})", "$.y"));
        decltype(rows) expected;
        expected.push_back(R"({"x":25})");
        REQUIRE(expected == rows);
    }
    SECTION("8") {
        auto rows = storage.select(json_remove<std::unique_ptr<std::string>>(R"({"x":25,"y":42})", "$"));
        REQUIRE(rows.size() == 1);
        REQUIRE(!bool(rows[0]));
    }
}

TEST_CASE("json_type") {
    auto storage = make_storage("");
    SECTION("1") {
        auto argument = R"({"a":[2,3.5,true,false,null,"x"]})";
        auto result = "object";
        SECTION("not null") {
            auto rows = storage.select(json_type(argument));
            decltype(rows) expected;
            expected.push_back(result);
            REQUIRE(expected == rows);
        }
        SECTION("null") {
            auto rows = storage.select(json_type<std::unique_ptr<std::string>>(argument));
            REQUIRE(rows.size() == 1);
            REQUIRE(rows[0]);
            REQUIRE(*rows[0] == result);
        }
    }
    SECTION("2") {
        struct TestCase {
            std::string argument;
            std::string result;
            std::string secondArgument;
        };
        std::vector<TestCase> testCases;
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "object", "$"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "array", "$.a"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "integer", "$.a[0]"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "real", "$.a[1]"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "true", "$.a[2]"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "false", "$.a[3]"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "null", "$.a[4]"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "text", "$.a[5]"});
        testCases.push_back(TestCase{R"({"a":[2,3.5,true,false,null,"x"]})", "", "$.a[6]"});
        for(auto& testCase: testCases) {
            {
                auto rows = storage.select(json_type(testCase.argument, testCase.secondArgument));
                decltype(rows) expected;
                expected.push_back(testCase.result);
                REQUIRE(expected == rows);
            }
            {
                auto rows =
                    storage.select(json_type<std::unique_ptr<std::string>>(testCase.argument, testCase.secondArgument));
                REQUIRE(rows.size() == 1);
                if(!testCase.result.empty()) {
                    REQUIRE(rows[0]);
                    REQUIRE(*rows[0] == testCase.result);
                } else {
                    REQUIRE(!rows[0]);
                }
            }
        }
    }
}

TEST_CASE("json_valid") {
    auto storage = make_storage("");
    struct TestCase {
        std::string argument;
        bool expected = false;
    };
    std::vector<TestCase> testCases;
    testCases.push_back(TestCase{R"({"x":35})", true});
    testCases.push_back(TestCase{R"({"x":35)", false});
    for(auto& testCase: testCases) {
        auto rows = storage.select(json_valid(testCase.argument));
        decltype(rows) expected;
        expected.push_back(testCase.expected);
        REQUIRE(rows == expected);
    }
}

TEST_CASE("json_quote") {
    auto storage = make_storage("");
    {
        auto rows = storage.select(json_quote<float>(3.14159));
        decltype(rows) expected;
        expected.push_back(3.14159);
        REQUIRE(rows == expected);
    }
    {
        auto rows = storage.select(json_quote<std::string>("verdant"));
        decltype(rows) expected;
        expected.push_back("\"verdant\"");
        REQUIRE(rows == expected);
    }
}

TEST_CASE("json_group_array && json_group_object") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storage =
        make_storage({}, make_table("users", make_column("id", &User::id), make_column("name", &User::name)));
    storage.sync_schema();

    storage.insert(User{1, "Bob"});
    storage.insert(User{2, "Alice"});

    SECTION("json_group_array") {
        {
            auto rows = storage.select(json_group_array(&User::id));
            decltype(rows) expected;
            expected.push_back("[1,2]");
            REQUIRE(rows == expected);
        }
        {
            auto rows = storage.select(json_group_array(&User::name));
            decltype(rows) expected;
            expected.push_back(R"(["Bob","Alice"])");
            REQUIRE(rows == expected);
        }
    }
    SECTION("json_group_object") {
        auto rows = storage.select(json_group_object(&User::id, &User::name));
        decltype(rows) expected;
        expected.push_back(R"({"1":"Bob","2":"Alice"})");
        REQUIRE(rows == expected);
    }
}
#endif  //  SQLITE_ENABLE_JSON1
