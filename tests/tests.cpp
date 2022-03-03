#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include <cassert>  //  assert
#include <vector>  //  std::vector
#include <string>  //  std::string
#include <memory>  //  std::unique_ptr
#include <cstdio>  //  remove
#include <numeric>  //  std::iota
#include <algorithm>  //  std::fill

using namespace sqlite_orm;

TEST_CASE("Limits") {
    auto storage2 = make_storage("limits.sqlite");
    auto storage = storage2;
    storage.sync_schema();

    {
        auto length = storage.limit.length();
        auto newLength = length - 10;
        storage.limit.length(newLength);
        length = storage.limit.length();
        REQUIRE(length == newLength);
    }
    {
        auto sqlLength = storage.limit.sql_length();
        auto newSqlLength = sqlLength - 10;
        storage.limit.sql_length(newSqlLength);
        sqlLength = storage.limit.sql_length();
        REQUIRE(sqlLength == newSqlLength);
    }
    {
        auto column = storage.limit.column();
        auto newColumn = column - 10;
        storage.limit.column(newColumn);
        column = storage.limit.column();
        REQUIRE(column == newColumn);
    }
    {
        auto exprDepth = storage.limit.expr_depth();
        auto newExprDepth = exprDepth - 10;
        storage.limit.expr_depth(newExprDepth);
        exprDepth = storage.limit.expr_depth();
        REQUIRE(exprDepth == newExprDepth);
    }
    {
        auto compoundSelect = storage.limit.compound_select();
        auto newCompoundSelect = compoundSelect - 10;
        storage.limit.compound_select(newCompoundSelect);
        compoundSelect = storage.limit.compound_select();
        REQUIRE(compoundSelect == newCompoundSelect);
    }
    {
        auto vdbeOp = storage.limit.vdbe_op();
        auto newVdbe_op = vdbeOp - 10;
        storage.limit.vdbe_op(newVdbe_op);
        vdbeOp = storage.limit.vdbe_op();
        REQUIRE(vdbeOp == newVdbe_op);
    }
    {
        auto functionArg = storage.limit.function_arg();
        auto newFunctionArg = functionArg - 10;
        storage.limit.function_arg(newFunctionArg);
        functionArg = storage.limit.function_arg();
        REQUIRE(functionArg == newFunctionArg);
    }
    {
        auto attached = storage.limit.attached();
        auto newAttached = attached - 1;
        storage.limit.attached(newAttached);
        attached = storage.limit.attached();
        REQUIRE(attached == newAttached);
    }
    {
        auto likePatternLength = storage.limit.like_pattern_length();
        auto newLikePatternLength = likePatternLength - 10;
        storage.limit.like_pattern_length(newLikePatternLength);
        likePatternLength = storage.limit.like_pattern_length();
        REQUIRE(likePatternLength == newLikePatternLength);
    }
    {
        auto variableNumber = storage.limit.variable_number();
        auto newVariableNumber = variableNumber - 10;
        storage.limit.variable_number(newVariableNumber);
        variableNumber = storage.limit.variable_number();
        REQUIRE(variableNumber == newVariableNumber);
    }
    {
        auto triggerDepth = storage.limit.trigger_depth();
        auto newTriggerDepth = triggerDepth - 10;
        storage.limit.trigger_depth(newTriggerDepth);
        triggerDepth = storage.limit.trigger_depth();
        REQUIRE(triggerDepth == newTriggerDepth);
    }
#if SQLITE_VERSION_NUMBER >= 3008007
    {
        auto workerThreads = storage.limit.worker_threads();
        auto newWorkerThreads = workerThreads + 1;
        storage.limit.worker_threads(newWorkerThreads);
        workerThreads = storage.limit.worker_threads();
        REQUIRE(workerThreads == newWorkerThreads);
    }
#endif
}

TEST_CASE("Custom collate") {
    struct Item {
        int id;
        std::string name;
    };

    struct OtotoCollation {
        int operator()(int leftLength, const void* lhs, int rightLength, const void* rhs) const {
            if(leftLength == rightLength) {
                return ::strncmp((const char*)lhs, (const char*)rhs, leftLength);
            } else {
                return 1;
            }
        }

        static const char* name() {
            return "ototo";
        }
    };

    struct AlwaysEqualCollation {
        int operator()(int leftLength, const void* lhs, int rightLength, const void* rhs) const {
            return 0;
        }

        static const char* name() {
            return "alwaysequal";
        }
    };

    auto useLegacyScript = false;
    SECTION("legacy API") {
        useLegacyScript = true;
    }
    SECTION("modern API") {
        useLegacyScript = false;
    }

    auto filename = "custom_collate.sqlite";
    ::remove(filename);
    auto storage = make_storage(
        filename,
        make_table("items", make_column("id", &Item::id, primary_key()), make_column("name", &Item::name)));
    storage.open_forever();
    storage.sync_schema();
    storage.remove_all<Item>();
    storage.insert(Item{0, "Mercury"});
    storage.insert(Item{0, "Mars"});
    if(useLegacyScript) {
        storage.create_collation("ototo", [](int leftLength, const void* lhs, int rightLength, const void* rhs) {
            if(leftLength == rightLength) {
                return ::strncmp((const char*)lhs, (const char*)rhs, leftLength);
            } else {
                return 1;
            }
        });
        storage.create_collation("alwaysequal", [](int, const void*, int, const void*) {
            return 0;
        });
    } else {
        storage.create_collation<OtotoCollation>();
        storage.create_collation<AlwaysEqualCollation>();
    }
    auto rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo")));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "Mercury");

    rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate<OtotoCollation>()));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "Mercury");

    rows = storage.select(&Item::name,
                          where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                          order_by(&Item::name).collate("ototo"));

    rows = storage.select(&Item::name,
                          where(is_equal(&Item::name, "Mercury").collate<AlwaysEqualCollation>()),
                          order_by(&Item::name).collate<OtotoCollation>());

    if(useLegacyScript) {
        storage.create_collation("ototo", {});
    } else {
        storage.delete_collation<OtotoCollation>();
    }
    try {
        rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo")));
        REQUIRE(false);
    } catch(const std::system_error&) {
        //        cout << e.what() << endl;
    }
    try {
        rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate<OtotoCollation>()));
        REQUIRE(false);
    } catch(const std::system_error&) {
        //        cout << e.what() << endl;
    }
    try {
        rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo2")));
        REQUIRE(false);
    } catch(const std::system_error&) {
        //        cout << e.what() << endl;
    }

    rows = storage.select(&Item::name,
                          where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                          order_by(&Item::name).collate_rtrim());
    REQUIRE(rows.size() == static_cast<size_t>(storage.count<Item>()));

    rows = storage.select(&Item::name,
                          where(is_equal(&Item::name, "Mercury").collate<AlwaysEqualCollation>()),
                          order_by(&Item::name).collate_rtrim());
    REQUIRE(rows.size() == static_cast<size_t>(storage.count<Item>()));

    rows = storage.select(&Item::name,
                          where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                          order_by(&Item::name).collate("alwaysequal"));
    REQUIRE(rows.size() == static_cast<size_t>(storage.count<Item>()));

    rows = storage.select(&Item::name,
                          where(is_equal(&Item::name, "Mercury").collate<AlwaysEqualCollation>()),
                          order_by(&Item::name).collate<AlwaysEqualCollation>());
    REQUIRE(rows.size() == static_cast<size_t>(storage.count<Item>()));
}

TEST_CASE("Vacuum") {
    struct Item {
        int id;
        std::string name;
    };

    auto storage = make_storage(
        "vacuum.sqlite",
        make_table("items", make_column("id", &Item::id, primary_key()), make_column("name", &Item::name)));
    storage.sync_schema();
    storage.insert(Item{0, "One"});
    storage.insert(Item{0, "Two"});
    storage.insert(Item{0, "Three"});
    storage.insert(Item{0, "Four"});
    storage.insert(Item{0, "Five"});
    storage.remove_all<Item>();
    storage.vacuum();
}
