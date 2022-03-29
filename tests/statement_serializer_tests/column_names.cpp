#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer column names") {
    SECTION("by member field pointer") {
        struct User {
            int id = 0;
            std::string name;
        };
        auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
        using storage_impl_t = internal::storage_impl<decltype(table)>;
        storage_impl_t storageImpl{table};
        {
            using context_t = internal::serializer_context<storage_impl_t>;
            context_t context{storageImpl};
            SECTION("id") {
                SECTION("skip table name") {
                    auto value = serialize(&User::id, context);
                    REQUIRE(value == R"("id")");
                }
                SECTION("don't skip table name") {
                    context.skip_table_name = false;
                    auto value = serialize(&User::id, context);
                    REQUIRE(value == R"("users"."id")");
                }
            }
            SECTION("name") {
                auto value = serialize(&User::name, context);
                REQUIRE(value == R"("name")");
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
        SECTION("getters, setters") {
            auto table = make_table("users",
                                    make_column("id", &User::getId, &User::setId),
                                    make_column("name", &User::getName, &User::setName));
            using storage_impl_t = internal::storage_impl<decltype(table)>;
            storage_impl_t storageImpl{table};
            using context_t = internal::serializer_context<storage_impl_t>;
            context_t context{storageImpl};
            std::string value;
            decltype(value) expected;
            SECTION("id") {
                SECTION("getter") {
                    value = serialize(&User::getId, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setId, context);
                }
                expected = R"("id")";
            }
            SECTION("name") {
                SECTION("getter") {
                    value = serialize(&User::getName, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setName, context);
                }
                expected = R"("name")";
            }
            REQUIRE(value == expected);
        }
        SECTION("setters, getters") {  //  column names by setters and getters pointers (reverse order)
            auto table = make_table("users",
                                    make_column("id", &User::setId, &User::getId),
                                    make_column("name", &User::setName, &User::getName));
            using storage_impl_t = internal::storage_impl<decltype(table)>;
            storage_impl_t storageImpl{table};
            using context_t = internal::serializer_context<storage_impl_t>;
            context_t context{storageImpl};
            std::string value;
            decltype(value) expected;
            SECTION("id") {
                SECTION("getter") {
                    value = serialize(&User::getId, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setId, context);
                }
                expected = R"("id")";
            }
            SECTION("name") {
                SECTION("getter") {
                    value = serialize(&User::getName, context);
                }
                SECTION("setter") {
                    value = serialize(&User::setName, context);
                }
                expected = R"("name")";
            }
            REQUIRE(value == expected);
        }
    }
    // note: `statement_serializer<column_pointer<>>` used `context.impl.column_name_simple(cp.field)` to look up the field name;
    //       this worked properly by chance under the following circumstances (yet still being undefined behaviour):
    //       - if the explicitly (derived) object would be the first mapped to storage (of the variadically derived storage_impl<>).
    //       - if the base class' field in question was mapped by multiple derived objects with the same column name.
    //       here we test whether the serializer finds the correct column by using `context.impl.column_name(cp)`
    SECTION("by explicit column pointer") {
        struct Object1 {
            int id = 0;
        };
        struct Object2 {
            int id = 0;
        };
        auto table1 = make_table("object1", make_column("id1", &Object1::id));
        auto table2 = make_table("object2", make_column("id2", &Object2::id));
        using storage_impl_t = internal::storage_impl<decltype(table1), decltype(table2)>;
        storage_impl_t storageImpl{table1, table2};
        {
            using context_t = internal::serializer_context<storage_impl_t>;
            context_t context{storageImpl};
            SECTION("name") {
                auto value = serialize(column<Object2>(&Object2::id), context);
                REQUIRE(value == R"("id2")");
            }
        }
    }
}
