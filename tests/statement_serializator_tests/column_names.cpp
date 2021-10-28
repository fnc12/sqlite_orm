#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator column names") {
    SECTION("by member field pointer") {
        struct User {
            int id = 0;
            std::string name;
        };
        auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
        using storage_impl_t = internal::storage_impl<decltype(table)>;
        storage_impl_t storageImpl{table};
        {
            using context_t = internal::serializator_context<storage_impl_t>;
            context_t context{storageImpl};
            SECTION("id") {
                SECTION("skip table name") {
                    auto value = serialize(&User::id, context);
                    REQUIRE(value == "\"id\"");
                }
                SECTION("don't skip table name") {
                    context.skip_table_name = false;
                    auto value = serialize(&User::id, context);
                    REQUIRE(value == "\"users\".\"id\"");
                }
            }
            SECTION("name") {
                auto value = serialize(&User::name, context);
                REQUIRE(value == "\"name\"");
            }
        }
    }
    SECTION("by getters and setters pointers") {
        struct User {

            int getId() const {
                return this->id;
            }

            void setId(int value) {
                this->id = value;
            }

            const std::string& getName() const {
                return this->name;
            }

            void setName(std::string value) {
                this->name = move(value);
            }

          private:
            int id = 0;
            std::string name;
        };
        {
            auto table = make_table("users",
                                    make_column("id", &User::getId, &User::setId),
                                    make_column("name", &User::getName, &User::setName));
            using storage_impl_t = internal::storage_impl<decltype(table)>;
            storage_impl_t storageImpl{table};
            {
                using context_t = internal::serializator_context<storage_impl_t>;
                context_t context{storageImpl};
                {
                    auto value = serialize(&User::getId, context);
                    REQUIRE(value == "\"id\"");
                }
                {
                    auto value = serialize(&User::setId, context);
                    REQUIRE(value == "\"id\"");
                }
                {
                    auto value = serialize(&User::getName, context);
                    REQUIRE(value == "\"name\"");
                }
                {
                    auto value = serialize(&User::setName, context);
                    REQUIRE(value == "\"name\"");
                }
            }
        }
        {  //  column names by setters and getters pointers (reverse order)
            auto table = make_table("users",
                                    make_column("id", &User::setId, &User::getId),
                                    make_column("name", &User::setName, &User::getName));
            using storage_impl_t = internal::storage_impl<decltype(table)>;
            storage_impl_t storageImpl{table};
            {
                using context_t = internal::serializator_context<storage_impl_t>;
                context_t context{storageImpl};
                {
                    auto value = serialize(&User::getId, context);
                    REQUIRE(value == "\"id\"");
                }
                {
                    auto value = serialize(&User::setId, context);
                    REQUIRE(value == "\"id\"");
                }
                {
                    auto value = serialize(&User::getName, context);
                    REQUIRE(value == "\"name\"");
                }
                {
                    auto value = serialize(&User::setName, context);
                    REQUIRE(value == "\"name\"");
                }
            }
        }
    }
}
