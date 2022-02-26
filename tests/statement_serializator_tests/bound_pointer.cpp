#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("bindable_pointer") {
    using internal::serialize;
    struct Dummy {};
    auto table = make_table<Dummy>("dummy");
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    std::string value;
    decltype(value) expected;
    SECTION("null by itself") {
        auto x = statically_bindable_pointer<carray_pvt, std::nullptr_t>(nullptr);
        value = serialize(x, context);
        expected = "null";
    }
    SECTION("null in select") {
        auto ast = select(statically_bindable_pointer<carray_pvt, std::nullptr_t>(nullptr));
        ast.highest_level = true;
        value = serialize(ast, context);
        expected = "SELECT null";
    }
    SECTION("null as function argument") {
        auto ast = func<remember_fn>(1, statically_bindable_pointer<carray_pvt, std::nullptr_t>(nullptr));
        value = serialize(ast, context);
        expected = "remember(1, null)";
    }
    SECTION("nullptr as function argument") {
        auto ast = func<remember_fn>(1, nullptr);
        value = serialize(ast, context);
        expected = "remember(1, null)";
    }

    REQUIRE(value == expected);
}
