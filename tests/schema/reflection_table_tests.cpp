#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <algorithm>  //  std::ranges::find

using namespace sqlite_orm;

namespace {
    struct[[= "plain"_orm_name]] ReflectedPlain {
        int64 id;
        std::string name;
    };

    struct ReflectedDefaultName {
        int64 id;
        std::string name;
    };

    struct[[= "annotated"_orm_name]] ReflectedAnnotated {
        [[= primary_key().autoincrement()]] int64 id;
        [[= not_null()]] std::string name;
        [[= default_value(0)]] int score;
        [[= collate_nocase()]] std::string handle;
    };

    struct[[= "composite"_orm_name]] ReflectedComposite {
        int a;
        int b;
        std::string note;
    };

    struct[[= "parent"_orm_name]] ReflectedParent {
        int64 id;
        std::string label;
    };

    struct[[= "child"_orm_name]] ReflectedChild {
        int64 id;
        int parentId;
    };

    // Class-scope annotations whose argument expressions reference T's own members
    // (composite `primary_key`, `foreign_key().references()`, multi-column `unique`)
    // are not expressible in C++26: the class-head annotation is parsed before T's
    // class-head-name enters scope. Until the language gains complete-class-context
    // parsing for class-head annotations or a deferred-lookup escape hatch, these
    // table-level constraints must come through the variadic-extras path (covered
    // above).
}

TEST_CASE("reflection-based make_table - name resolution") {
    SECTION("[[=orm_name(...)]] annotation supplies the table name") {
        auto table = make_table<ReflectedPlain>();
        REQUIRE(table.name == "plain");
    }

    SECTION("fallback to type identifier") {
        auto table = make_table<ReflectedDefaultName>();
        REQUIRE(table.name == "ReflectedDefaultName");
    }
}

TEST_CASE("reflection-based make_table - column reflection") {
    auto table = make_table<ReflectedPlain>();
    STATIC_REQUIRE(table.template count_of<internal::is_column>() == 2);
    REQUIRE(*table.find_column_name(&ReflectedPlain::id) == "id");
    REQUIRE(*table.find_column_name(&ReflectedPlain::name) == "name");
}

TEST_CASE("reflection-based make_table - member annotations") {
    auto table = make_table<ReflectedAnnotated>();
    STATIC_REQUIRE(table.template count_of<internal::is_column>() == 4);
    REQUIRE(*table.find_column_name(&ReflectedAnnotated::id) == "id");
    REQUIRE(*table.find_column_name(&ReflectedAnnotated::name) == "name");
    REQUIRE(*table.find_column_name(&ReflectedAnnotated::score) == "score");
    REQUIRE(*table.find_column_name(&ReflectedAnnotated::handle) == "handle");

    auto storage = make_storage("", std::move(table));
    REQUIRE_NOTHROW(storage.sync_schema());
    auto info = storage.pragma.table_xinfo("annotated");
    REQUIRE(info.size() == 4);

    auto findCol = [&info](const std::string& name) -> const table_xinfo& {
        auto it = std::ranges::find(info, name, &table_xinfo::name);
        REQUIRE(it != info.end());
        return *it;
    };

    SECTION("primary_key().autoincrement() applies to the annotated member") {
        REQUIRE(findCol("id").pk == 1);
    }

    SECTION("not_null() applies to the annotated member") {
        REQUIRE(findCol("name").notnull == true);
    }

    SECTION("default_value() with a literal-type value applies to the annotated member") {
        REQUIRE(findCol("score").dflt_value == "0");
    }
}

TEST_CASE("reflection-based make_table - variadic extras") {
    SECTION("composite primary key supplied as an extra") {
        auto table = make_table<ReflectedComposite>(primary_key(&ReflectedComposite::a, &ReflectedComposite::b));
        auto names = table.table_key_columns_names();
        REQUIRE(names.size() == 2);
        REQUIRE(names[0] == "a");
        REQUIRE(names[1] == "b");
    }

    SECTION("foreign_key().references() supplied as an extra survives schema sync") {
        auto storage = make_storage(
            "",
            make_table<ReflectedParent>(),
            make_table<ReflectedChild>(foreign_key(&ReflectedChild::parentId).references(&ReflectedParent::id)));
        REQUIRE_NOTHROW(storage.sync_schema());

        auto child = storage.pragma.table_xinfo("child");
        REQUIRE(child.size() == 2);
    }
}

TEST_CASE("reflection-based make_table - overload dispatch") {
    SECTION("classical overload chosen when a column type is passed") {
        auto classical = make_table<ReflectedPlain>("u", make_column("id", &ReflectedPlain::id));
        using elements = typename decltype(classical)::elements_type;
        STATIC_REQUIRE(std::tuple_size_v<elements> == 1);
        REQUIRE(classical.name == "u");
    }

    SECTION("reflection overload chosen when no columns are passed") {
        auto reflected = make_table<ReflectedPlain>();
        using elements = typename decltype(reflected)::elements_type;
        STATIC_REQUIRE(std::tuple_size_v<elements> == 2);
        REQUIRE(reflected.name == "plain");
    }
}
#endif
