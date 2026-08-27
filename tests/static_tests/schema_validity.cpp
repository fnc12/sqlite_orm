#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <tuple>  //  std::ignore

using namespace sqlite_orm;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };

    struct Visit {
        int id = 0;
        int userId = 0;
    };
}

TEST_CASE("schema factory arguments are validated at compile time") {
    SECTION("database objects are recognized") {
        using Table = decltype(make_table("users",
                                          make_column("id", &User::id, primary_key()),
                                          make_column("name", &User::name)));
        using Index = decltype(make_index("idx_users_name", &User::name));
        using Trigger = decltype(make_trigger("trigger", after().insert().on<User>().begin(select(&User::id)).end()));
        STATIC_REQUIRE(internal::is_database_object<Table>::value);
        STATIC_REQUIRE(internal::is_database_object<Index>::value);
        STATIC_REQUIRE(internal::is_database_object<Trigger>::value);
    }
    SECTION("other types are not database objects") {
        STATIC_REQUIRE_FALSE(internal::is_database_object<User>::value);
        STATIC_REQUIRE_FALSE(internal::is_database_object<int>::value);
        STATIC_REQUIRE_FALSE(internal::is_database_object<decltype(where(c(&User::id) > 0))>::value);
        //  a storage option is admissible in `make_storage()`, but it is not a database object
        STATIC_REQUIRE_FALSE(internal::is_database_object<connection_control>::value);
    }
    //  the index factories gate on these predicates; the constructions guard against over-strictness
    SECTION("an index element with a derivable table type must belong to the table of the index") {
        STATIC_REQUIRE(internal::is_index_element_of_v<decltype(&User::name), User>);
        STATIC_REQUIRE(internal::is_index_element_of_v<decltype(indexed_column(&User::name).desc()), User>);
        //  expressions and a partial-index WHERE carry no table type and are admitted
        STATIC_REQUIRE(internal::is_index_element_of_v<decltype(lower(&User::name)), User>);
        STATIC_REQUIRE(internal::is_index_element_of_v<decltype(where(c(&User::id) > 0)), User>);
        STATIC_REQUIRE_FALSE(internal::is_index_element_of_v<decltype(&Visit::userId), User>);
        STATIC_REQUIRE_FALSE(internal::is_index_element_of_v<decltype(indexed_column(&Visit::userId)), User>);
    }
    SECTION("a trigger body statement must be a complete statement") {
        STATIC_REQUIRE(internal::is_object_dml_expression_v<decltype(insert(User{}))>);
        STATIC_REQUIRE(internal::is_object_dml_expression_v<decltype(update(User{}))>);
        STATIC_REQUIRE(internal::is_object_dml_expression_v<decltype(replace(User{}))>);
        STATIC_REQUIRE(internal::is_object_dml_expression_v<decltype(remove<User>(1))>);
        STATIC_REQUIRE(internal::is_raw_dml_expression_v<decltype(update_all(set(c(&User::name) = "")))>);
        STATIC_REQUIRE(internal::is_raw_dml_expression_v<decltype(remove_all<User>())>);
        STATIC_REQUIRE(internal::is_select_expression_v<decltype(select(&User::id))>);
        //  fragments of a statement are not statements
        STATIC_REQUIRE_FALSE(internal::is_object_dml_expression_v<decltype(get<User>(1))>);
        STATIC_REQUIRE_FALSE(internal::is_object_dml_expression_v<decltype(where(c(&User::id) > 0))>);
        STATIC_REQUIRE_FALSE(internal::is_object_dml_expression_v<int>);
    }
    SECTION("frame boundaries keep their grammar positions") {
        STATIC_REQUIRE(internal::is_frame_start_bound_v<internal::unbounded_preceding_t>);
        STATIC_REQUIRE(internal::is_frame_start_bound_v<decltype(preceding(1))>);
        STATIC_REQUIRE(internal::is_frame_start_bound_v<internal::current_row_t>);
        STATIC_REQUIRE(internal::is_frame_start_bound_v<decltype(following(1))>);
        STATIC_REQUIRE_FALSE(internal::is_frame_start_bound_v<internal::unbounded_following_t>);
        STATIC_REQUIRE(internal::is_frame_end_bound_v<internal::unbounded_following_t>);
        STATIC_REQUIRE(internal::is_frame_end_bound_v<decltype(preceding(1))>);
        STATIC_REQUIRE_FALSE(internal::is_frame_end_bound_v<internal::unbounded_preceding_t>);
        STATIC_REQUIRE_FALSE(internal::is_frame_end_bound_v<int>);
    }
    //  These constructions have to keep compiling: they exercise every schema factory this batch gated,
    //  so they are the regression guard against the new asserts being over-strict.
    SECTION("gated factories accept their whole legal envelope") {
        //  an expression index cannot deduce its table, so the explicit-table overload spells it
        std::ignore = make_index<User>("idx_expr", lower(&User::name));
        std::ignore = make_index("idx_partial", &User::name, where(length(&User::name) > 2));
        std::ignore = make_unique_index("idx_unique", indexed_column(&User::name).collate("BINARY").desc());
        std::ignore = make_index<User>("idx_explicit", &User::id, &User::name);
        std::ignore = make_trigger("trg",
                                   after()
                                       .update_of(&User::name, column<User>(&User::id))
                                       .on<User>()
                                       .when(greater_than(new_(&User::id), 0))
                                       .begin(update_all(set(c(&User::name) = "")), select(&User::id))
                                       .end());
        std::ignore = partition_by(&User::id, add(&User::id, 1));
        std::ignore = rows(unbounded_preceding(), current_row());
        std::ignore = range(preceding(1), following(1));
        std::ignore = groups(current_row(), unbounded_following());
    }
}
