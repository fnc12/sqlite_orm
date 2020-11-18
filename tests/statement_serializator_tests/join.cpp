#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator join") {
    /*struct User {
        int id = 0;
    };
    struct Visit {
        int id = 0;
        int userId = 0;
    };
    auto usersTable = make_table("users",
                                 make_column("id", &User::id, primary_key()));
    auto visitsTable = make_index("visits",
                                  make_column("id", &Visit::id, primary_key()),
                                  make_column("user_id", &Visit::userId));
    

    using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

    storage_impl_t storageImpl{usersTable, visitsTable};

    using context_t = internal::serializator_context<storage_impl_t>;
    
    context_t context{storageImpl};
    
    auto j = join<Visit>(on(is_equal(&Visit::userId, &User::id)));
    auto value = internal::serialize(j, context);
    REQUIRE(value == "123");*/
}
