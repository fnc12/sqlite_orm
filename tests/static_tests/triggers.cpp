#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("trigger body node classification") {
    using internal::is_new_row_ref_v;
    using internal::is_old_row_ref_v;
    using internal::is_operator_argument_v;
    using internal::is_raise_v;

    struct User {
        int id = 0;
        std::string name;
    };

    using OldRef = decltype(old(&User::id));
    using NewRef = decltype(new_(&User::id));
    using Raise = decltype(raise_rollback("boom"));

    SECTION("OLD") {
        STATIC_REQUIRE(is_old_row_ref_v<OldRef>);
        STATIC_REQUIRE_FALSE(is_new_row_ref_v<OldRef>);
        STATIC_REQUIRE_FALSE(is_raise_v<OldRef>);
    }
    SECTION("NEW") {
        STATIC_REQUIRE(is_new_row_ref_v<NewRef>);
        STATIC_REQUIRE_FALSE(is_old_row_ref_v<NewRef>);
        STATIC_REQUIRE_FALSE(is_raise_v<NewRef>);
    }
    SECTION("RAISE") {
        //  every RAISE flavour is the one node type
        STATIC_REQUIRE(is_raise_v<Raise>);
        STATIC_REQUIRE(is_raise_v<decltype(raise_ignore())>);
        STATIC_REQUIRE(is_raise_v<decltype(raise_abort("boom"))>);
        STATIC_REQUIRE(is_raise_v<decltype(raise_fail("boom"))>);
    }
    SECTION("row references are operands, RAISE is not") {
        STATIC_REQUIRE(is_operator_argument_v<OldRef>);
        STATIC_REQUIRE(is_operator_argument_v<NewRef>);
        STATIC_REQUIRE_FALSE(is_operator_argument_v<Raise>);
    }
    SECTION("unrelated node") {
        using Unrelated = decltype(&User::name);
        STATIC_REQUIRE_FALSE(is_old_row_ref_v<Unrelated>);
        STATIC_REQUIRE_FALSE(is_new_row_ref_v<Unrelated>);
        STATIC_REQUIRE_FALSE(is_raise_v<Unrelated>);
    }
}

TEST_CASE("trigger specification classification") {
    using internal::is_trigger_event_spec_v;
    using internal::is_trigger_event_v;
    using internal::is_trigger_spec_v;
    using internal::is_trigger_timing_v;
    using internal::is_trigger_update_of_v;
    using internal::is_trigger_v;

    struct User {
        int id = 0;
        std::string name;
    };

    using Timing = internal::trigger_timing;
    using Event = internal::trigger_type;
    using EventSpec = decltype(after().insert());
    using UpdateOf = decltype(after().update_of(&User::name));
    using Spec = decltype(after().insert().on<User>());
    using Trigger = decltype(make_trigger("trg", after().insert().on<User>().begin(select(1)).end()));

    SECTION("keywords") {
        STATIC_REQUIRE(is_trigger_timing_v<Timing>);
        STATIC_REQUIRE_FALSE(is_trigger_event_v<Timing>);
        STATIC_REQUIRE(is_trigger_event_v<Event>);
        STATIC_REQUIRE_FALSE(is_trigger_timing_v<Event>);
    }
    SECTION("event spec") {
        STATIC_REQUIRE(is_trigger_event_spec_v<EventSpec>);
        STATIC_REQUIRE_FALSE(is_trigger_update_of_v<EventSpec>);
    }
    SECTION("UPDATE OF is a refinement of the event spec") {
        STATIC_REQUIRE(is_trigger_event_spec_v<UpdateOf>);
        STATIC_REQUIRE(is_trigger_update_of_v<UpdateOf>);
    }
    SECTION("trigger spec") {
        STATIC_REQUIRE(is_trigger_spec_v<Spec>);
        STATIC_REQUIRE_FALSE(is_trigger_event_spec_v<Spec>);
        //  a trigger specification is not yet a trigger - it carries neither name nor body
        STATIC_REQUIRE_FALSE(is_trigger_v<Spec>);
        STATIC_REQUIRE(is_trigger_v<Trigger>);
        STATIC_REQUIRE_FALSE(is_trigger_spec_v<Trigger>);
    }
}
