#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator primary key") {
    {  //  empty pk
        internal::serializator_context_base context;
        auto pk = primary_key();
        auto value = serialize(pk, context);
        REQUIRE(value == "PRIMARY KEY");
    }
    {
        struct User {
            int id = 0;
            std::string name;
        };
        auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
        using storage_impl_t = internal::storage_impl<decltype(table)>;
        auto storageImpl = storage_impl_t{table};
        using context_t = internal::serializator_context<storage_impl_t>;
        context_t context{storageImpl};
        {  //  single column pk
            auto pk = primary_key(&User::id);
            auto value = serialize(pk, context);
            REQUIRE(value == "PRIMARY KEY(id)");
        }
        {  //  double column pk
            auto pk = primary_key(&User::id, &User::name);
            auto value = serialize(pk, context);
            REQUIRE(value == "PRIMARY KEY(id, name)");
        }
        {  //  empty pk asc
            auto pk = primary_key().asc();
            auto value = serialize(pk, context);
            REQUIRE(value == "PRIMARY KEY ASC");
        }
        {  //  empty pk desc
            auto pk = primary_key().desc();
            auto value = serialize(pk, context);
            REQUIRE(value == "PRIMARY KEY DESC");
        }
    }
}
