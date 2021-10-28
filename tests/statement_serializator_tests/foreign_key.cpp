#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

#if SQLITE_VERSION_NUMBER >= 3006019

TEST_CASE("statement_serializator foreign key") {
    SECTION("one to one") {
        struct User {
            int id = 0;
            std::string name;
        };

        struct Visit {
            int id = 0;
            decltype(User::id) userId;
            long time = 0;
        };

        auto usersTable = make_table("users",
                                     make_column("id", &User::id, primary_key(), autoincrement()),
                                     make_column("name", &User::name));

        SECTION("simple") {
            auto fk = foreign_key(&Visit::userId).references(&User::id);
            {
                using ForeignKey = decltype(fk);
                static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
            }
            auto visitsTable = make_table("visits",
                                          make_column("id", &Visit::id, primary_key(), autoincrement()),
                                          make_column("user_id", &Visit::userId),
                                          make_column("time", &Visit::time),
                                          fk);

            using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

            storage_impl_t storageImpl{usersTable, visitsTable};

            using context_t = internal::serializator_context<storage_impl_t>;

            context_t context{storageImpl};
            auto value = serialize(fk, context);
            REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id')");
        }
        SECTION("on update") {
            SECTION("no_action") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_update.no_action();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON UPDATE NO ACTION");
            }
            SECTION("restrict_") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_update.restrict_();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON UPDATE RESTRICT");
            }
            SECTION("set_null") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_update.set_null();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON UPDATE SET NULL");
            }
            SECTION("set_default") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_update.set_default();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON UPDATE SET DEFAULT");
            }
            SECTION("cascade") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_update.cascade();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON UPDATE CASCADE");
            }
        }
        SECTION("on delete") {
            SECTION("no_action") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_delete.no_action();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON DELETE NO ACTION");
            }
            SECTION("restrict_") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_delete.restrict_();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON DELETE RESTRICT");
            }
            SECTION("set_null") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_delete.set_null();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON DELETE SET NULL");
            }
            SECTION("set_default") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_delete.set_default();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON DELETE SET DEFAULT");
            }
            SECTION("cascade") {
                auto fk = foreign_key(&Visit::userId).references(&User::id).on_delete.cascade();
                {
                    using ForeignKey = decltype(fk);
                    static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
                }
                auto visitsTable = make_table("visits",
                                              make_column("id", &Visit::id, primary_key(), autoincrement()),
                                              make_column("user_id", &Visit::userId),
                                              make_column("time", &Visit::time),
                                              fk);

                using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

                storage_impl_t storageImpl{usersTable, visitsTable};

                using context_t = internal::serializator_context<storage_impl_t>;

                context_t context{storageImpl};
                auto value = serialize(fk, context);
                REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id') ON DELETE CASCADE");
            }
        }
    }
    SECTION("one to explicit one") {
        struct Object {
            int id = 0;
        };

        struct User : Object {
            std::string name;

            User(decltype(id) id_, decltype(name) name_) : Object{id_}, name(move(name_)) {}
        };

        struct Token : Object {
            std::string token;
            int usedId = 0;

            Token(decltype(id) id_, decltype(token) token_, decltype(usedId) usedId_) :
                Object{id_}, token(std::move(token_)), usedId(usedId_) {}
        };
        auto fk = foreign_key(&Token::usedId).references(column<User>(&User::id));
        {
            using ForeignKey = decltype(fk);
            static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
        }
        auto usersTable =
            make_table<User>("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
        auto tokensTable = make_table<Token>("tokens",
                                             make_column("id", &Token::id, primary_key()),
                                             make_column("token", &Token::token),
                                             make_column("user_id", &Token::usedId),
                                             fk);

        using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(tokensTable)>;

        storage_impl_t storageImpl{usersTable, tokensTable};

        using context_t = internal::serializator_context<storage_impl_t>;

        context_t context{storageImpl};
        auto value = serialize(fk, context);
        REQUIRE(value == "FOREIGN KEY('user_id') REFERENCES 'users'('id')");
    }
    SECTION("composite key") {
        struct User {
            int id = 0;
            std::string firstName;
            std::string lastName;
        };

        struct UserVisit {
            int userId = 0;
            std::string userFirstName;
            time_t time = 0;
        };

        auto fk = foreign_key(&UserVisit::userId, &UserVisit::userFirstName).references(&User::id, &User::firstName);
        {
            using ForeignKey = decltype(fk);
            static_assert(std::is_same<ForeignKey::target_type, User>::value, "");
        }
        auto usersTable = make_table("users",
                                     make_column("id", &User::id),
                                     make_column("first_name", &User::firstName),
                                     make_column("last_name", &User::lastName),
                                     primary_key(&User::id, &User::firstName));
        {
            static_assert(internal::storage_traits::table_foreign_keys_count<decltype(usersTable), User>::value == 0,
                          "");
            static_assert(internal::storage_traits::table_foreign_keys_count<decltype(usersTable), UserVisit>::value ==
                              0,
                          "");
        }
        auto visitsTable = make_table("visits",
                                      make_column("user_id", &UserVisit::userId),
                                      make_column("user_first_name", &UserVisit::userFirstName),
                                      make_column("time", &UserVisit::time),
                                      fk);
        {
            static_assert(internal::storage_traits::table_foreign_keys_count<decltype(visitsTable), User>::value == 1,
                          "");
            static_assert(internal::storage_traits::table_foreign_keys_count<decltype(visitsTable), UserVisit>::value ==
                              0,
                          "");
        }
        using storage_impl_t = internal::storage_impl<decltype(usersTable), decltype(visitsTable)>;

        storage_impl_t storageImpl{usersTable, visitsTable};

        using context_t = internal::serializator_context<storage_impl_t>;

        context_t context{storageImpl};
        auto value = serialize(fk, context);
        REQUIRE(value == "FOREIGN KEY('user_id', 'user_first_name') REFERENCES 'users'('id', 'first_name')");
    }
}

#endif  //  SQLITE_VERSION_NUMBER
