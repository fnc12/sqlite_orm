#include <cstdint>
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("Current time/date/timestamp") {
    auto storage = make_storage("");
    SECTION("time") {
        SECTION("strict") {
            REQUIRE(storage.current_time().size());
        }
        SECTION("select") {
            auto rows = storage.select(current_time());
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.at(0).size());
        }
        SECTION("prepared statement") {
            auto preparedStatement = storage.prepare(select(current_time()));
            auto rows = storage.execute(preparedStatement);
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.at(0).size());
        }
    }
    SECTION("date") {
        SECTION("strict") {
            REQUIRE(storage.current_date().size());
        }
        SECTION("select") {
            auto rows = storage.select(current_date());
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.at(0).size());
        }
        SECTION("prepared statement") {
            auto preparedStatement = storage.prepare(select(current_date()));
            auto rows = storage.execute(preparedStatement);
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.at(0).size());
        }
    }
    SECTION("timestamp") {
        SECTION("strict") {
            REQUIRE(storage.current_timestamp().size());
        }
        SECTION("select") {
            auto rows = storage.select(current_timestamp());
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.at(0).size());
        }
        SECTION("prepared statement") {
            auto preparedStatement = storage.prepare(select(current_timestamp()));
            auto rows = storage.execute(preparedStatement);
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.at(0).size());
        }
    }
}

TEST_CASE("busy handler") {
    auto storage = make_storage({});
    storage.busy_handler([](int /*timesCount*/) {
        return 0;
    });
}

TEST_CASE("drop table") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        std::string date;
    };
    const std::string usersTableName = "users";
    const std::string visitsTableName = "visits";
    auto storage = make_storage(
        {},
        make_table(usersTableName, make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
        make_table(visitsTableName, make_column("id", &Visit::id, primary_key()), make_column("date", &Visit::date)));
    REQUIRE_FALSE(storage.table_exists(usersTableName));
    REQUIRE_FALSE(storage.table_exists(visitsTableName));

    storage.sync_schema();
    REQUIRE(storage.table_exists(usersTableName));
    REQUIRE(storage.table_exists(visitsTableName));

    storage.drop_table(usersTableName);
    REQUIRE_FALSE(storage.table_exists(usersTableName));
    REQUIRE(storage.table_exists(visitsTableName));

    storage.drop_table(visitsTableName);
    REQUIRE_FALSE(storage.table_exists(usersTableName));
    REQUIRE_FALSE(storage.table_exists(visitsTableName));

    REQUIRE_THROWS(storage.drop_table(usersTableName));
    REQUIRE_THROWS(storage.drop_table(visitsTableName));

    REQUIRE_NOTHROW(storage.drop_table_if_exists(usersTableName));
    REQUIRE_NOTHROW(storage.drop_table_if_exists(visitsTableName));
}

TEST_CASE("drop index") {
    struct User {
        int id = 0;
        std::string name;
    };
    const std::string indexName = "user_id_index";
    auto storage = make_storage(
        {},
        make_index("user_id_index", &User::id),
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    REQUIRE_NOTHROW(storage.drop_index(indexName));
    REQUIRE_THROWS(storage.drop_index(indexName));
    REQUIRE_NOTHROW(storage.drop_index_if_exists(indexName));
}

TEST_CASE("drop trigger") {
    struct User {
        int id = 0;
        std::string name;
    };
    const std::string triggerName = "table_insert_InsertTest";
    auto storage = make_storage(
        {},
        make_trigger(triggerName, after().insert().on<User>().begin(update_all(set(c(&User::id) = 5))).end()),
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();
    REQUIRE_NOTHROW(storage.drop_trigger(triggerName));
    REQUIRE_THROWS(storage.drop_trigger(triggerName));
    REQUIRE_NOTHROW(storage.drop_trigger_if_exists(triggerName));
}

TEST_CASE("rename table") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        std::string date;
    };
    const std::string usersTableName = "users";
    const std::string userNewTableName = "users_new";
    const std::string visitsTableName = "visits";
    const std::string visitsNewTableName = "visits_new";
    auto storage = make_storage(
        {},
        make_table(usersTableName, make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
        make_table(visitsTableName, make_column("id", &Visit::id, primary_key()), make_column("date", &Visit::date)));

    REQUIRE_FALSE(storage.table_exists(usersTableName));
    REQUIRE_FALSE(storage.table_exists(userNewTableName));
    REQUIRE_FALSE(storage.table_exists(visitsTableName));
    REQUIRE_FALSE(storage.table_exists(visitsNewTableName));
    REQUIRE(storage.tablename<User>() == usersTableName);
    REQUIRE(storage.tablename<User>() != userNewTableName);
    REQUIRE(storage.tablename<Visit>() == visitsTableName);
    REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

    storage.sync_schema();
    REQUIRE(storage.table_exists(usersTableName));
    REQUIRE_FALSE(storage.table_exists(userNewTableName));
    REQUIRE(storage.table_exists(visitsTableName));
    REQUIRE_FALSE(storage.table_exists(visitsNewTableName));
    REQUIRE(storage.tablename<User>() == usersTableName);
    REQUIRE(storage.tablename<User>() != userNewTableName);
    REQUIRE(storage.tablename<Visit>() == visitsTableName);
    REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

    SECTION("with 1 argument") {
        storage.rename_table<User>(userNewTableName);
        REQUIRE(storage.table_exists(usersTableName));
        REQUIRE_FALSE(storage.table_exists(userNewTableName));
        REQUIRE(storage.table_exists(visitsTableName));
        REQUIRE_FALSE(storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() != usersTableName);
        REQUIRE(storage.tablename<User>() == userNewTableName);
        REQUIRE(storage.tablename<Visit>() == visitsTableName);
        REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

        storage.rename_table<Visit>(visitsNewTableName);
        REQUIRE(storage.table_exists(usersTableName));
        REQUIRE_FALSE(storage.table_exists(userNewTableName));
        REQUIRE(storage.table_exists(visitsTableName));
        REQUIRE_FALSE(storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() != usersTableName);
        REQUIRE(storage.tablename<User>() == userNewTableName);
        REQUIRE(storage.tablename<Visit>() != visitsTableName);
        REQUIRE(storage.tablename<Visit>() == visitsNewTableName);
    }
    SECTION("with 2 arguments") {

        storage.rename_table(usersTableName, userNewTableName);
        REQUIRE_FALSE(storage.table_exists(usersTableName));
        REQUIRE(storage.table_exists(userNewTableName));
        REQUIRE(storage.table_exists(visitsTableName));
        REQUIRE_FALSE(storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() == usersTableName);
        REQUIRE(storage.tablename<User>() != userNewTableName);
        REQUIRE(storage.tablename<Visit>() == visitsTableName);
        REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

        storage.rename_table(visitsTableName, visitsNewTableName);
        REQUIRE_FALSE(storage.table_exists(usersTableName));
        REQUIRE(storage.table_exists(userNewTableName));
        REQUIRE_FALSE(storage.table_exists(visitsTableName));
        REQUIRE(storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() == usersTableName);
        REQUIRE(storage.tablename<User>() != userNewTableName);
        REQUIRE(storage.tablename<Visit>() == visitsTableName);
        REQUIRE(storage.tablename<Visit>() != visitsNewTableName);
    }
}

TEST_CASE("Storage copy") {
    struct User {
        int id = 0;
    };

    int calledCount = 0;

    auto storage = make_storage({}, make_table("users", make_column("id", &User::id)));
    storage.sync_schema();
    storage.remove_all<User>();

    storage.on_open = [&calledCount](sqlite3*) {
        ++calledCount;
    };

    storage.on_open(nullptr);
    REQUIRE(calledCount == 1);

    auto storageCopy = storage;
    REQUIRE(storageCopy.on_open);
    REQUIRE(calledCount == 2);
    storageCopy.on_open(nullptr);
    REQUIRE(calledCount == 3);

    storageCopy.sync_schema();
    storageCopy.remove_all<User>();
}

#if SQLITE_VERSION_NUMBER >= 3006019
TEST_CASE("column_name") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        int userId = 0;
        int date = 0;

        int notUsed = 0;
    };
    auto storage =
        make_storage({},
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
                     make_table("visits",
                                make_column("id", &Visit::id, primary_key()),
                                make_column("user_id", &Visit::userId),
                                make_column("date", &Visit::date),
                                foreign_key(&Visit::userId).references(&User::id)));
    REQUIRE(*storage.find_column_name(&User::id) == "id");
    REQUIRE(*storage.find_column_name(&User::name) == "name");
    REQUIRE(*storage.find_column_name(&Visit::id) == "id");
    REQUIRE(*storage.find_column_name(&Visit::userId) == "user_id");
    REQUIRE(*storage.find_column_name(&Visit::date) == "date");
    REQUIRE(storage.find_column_name(&Visit::notUsed) == nullptr);
}
#endif

namespace {
    class Record final {
      public:
        using ID = std::uint64_t;
        using TimeMs = std::uint64_t;

        ID id() const noexcept {
            return m_id;
        }
        void setId(ID val) noexcept {
            m_id = val;
        }

        TimeMs time() const noexcept {
            return m_time;
        }
        void setTime(const TimeMs& val) noexcept {
            m_time = val;
        }

      private:
        ID m_id{};
        TimeMs m_time{};
    };
}
TEST_CASE("non-unique DBOs") {
    auto idx1 = make_unique_index("idx_record_id", &Record::id);
    auto idx2 = make_index("idx_record_time", &Record::time);
    static_assert(std::is_same<decltype(idx1), decltype(idx2)>::value, "");
    auto db = make_storage({},
                           idx1,
                           idx2,
                           make_table("record",
                                      make_column("id", &Record::setId, &Record::id),
                                      make_column("time", &Record::setTime, &Record::time)));
    db.sync_schema();
}

TEST_CASE("insert") {
    struct Object {
        int id;
        std::string name;
    };

    struct ObjectWithoutRowid {
        int id;
        std::string name;
    };

    auto storage = make_storage(
        "test_insert.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)),
        make_table("objects_without_rowid",
                   make_column("id", &ObjectWithoutRowid::id, primary_key()),
                   make_column("name", &ObjectWithoutRowid::name))
            .without_rowid());

    storage.sync_schema();
    storage.remove_all<Object>();
    storage.remove_all<ObjectWithoutRowid>();

    for(auto i = 0; i < 100; ++i) {
        storage.insert(Object{
            0,
            "Skillet",
        });
        REQUIRE(storage.count<Object>() == i + 1);
    }

    auto initList = {
        Object{
            0,
            "Insane",
        },
        Object{
            0,
            "Super",
        },
        Object{
            0,
            "Sun",
        },
    };

    auto countBefore = storage.count<Object>();
    SECTION("straight") {
        storage.insert_range(initList.begin(), initList.end());
        REQUIRE(storage.count<Object>() == countBefore + static_cast<int>(initList.size()));

        //  test empty container
        std::vector<Object> emptyVector;
        REQUIRE_NOTHROW(storage.insert_range(emptyVector.begin(), emptyVector.end()));
    }
    SECTION("pointers") {
        std::vector<std::unique_ptr<Object>> pointers;
        pointers.reserve(initList.size());
        std::transform(initList.begin(), initList.end(), std::back_inserter(pointers), [](const Object& object) {
            return std::make_unique<Object>(Object{object});
        });
        storage.insert_range(pointers.begin(), pointers.end(), &std::unique_ptr<Object>::operator*);

        //  test empty container
        std::vector<std::unique_ptr<Object>> emptyVector;
        REQUIRE_NOTHROW(
            storage.insert_range(emptyVector.begin(), emptyVector.end(), &std::unique_ptr<Object>::operator*));
    }

    //  test insert without rowid
    storage.insert(ObjectWithoutRowid{10, "Life"});
    REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
    storage.insert(ObjectWithoutRowid{20, "Death"});
    REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
}

TEST_CASE("Empty storage") {
    auto storage = make_storage("empty.sqlite");
    storage.table_exists("table");
}

TEST_CASE("Remove") {
    struct Object {
        int id;
        std::string name;
    };

    {
        auto storage = make_storage(
            "test_remove.sqlite",
            make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));

        storage.sync_schema();
        storage.remove_all<Object>();

        auto id1 = storage.insert(Object{0, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(id1);
        REQUIRE(storage.count<Object>() == 0);
    }
    {
        auto storage = make_storage("test_remove.sqlite",
                                    make_table("objects",
                                               make_column("id", &Object::id),
                                               make_column("name", &Object::name),
                                               primary_key(&Object::id)));
        storage.sync_schema();
        storage.remove_all<Object>();

        auto id1 = storage.insert(Object{0, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(id1);
        REQUIRE(storage.count<Object>() == 0);
    }
    {
        auto storage = make_storage("",
                                    make_table("objects",
                                               make_column("id", &Object::id),
                                               make_column("name", &Object::name),
                                               primary_key(&Object::id, &Object::name)));
        storage.sync_schema();
        storage.replace(Object{1, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(1, "Skillet");
        REQUIRE(storage.count<Object>() == 0);

        storage.replace(Object{1, "Skillet"});
        storage.replace(Object{2, "Paul Cless"});
        REQUIRE(storage.count<Object>() == 2);
        storage.remove<Object>(1, "Skillet");
        REQUIRE(storage.count<Object>() == 1);
    }
}

#if SQLITE_VERSION_NUMBER >= 3031000
TEST_CASE("insert with generated column") {
    struct Product {
        std::string name;
        double price = 0;
        double discount = 0;
        double tax = 0;
        double netPrice = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Product() = default;
        Product(std::string name, double price, double discount, double tax, double netPrice) :
            name{std::move(name)}, price{price}, discount{discount}, tax{tax}, netPrice{netPrice} {}
#endif

        bool operator==(const Product& other) const {
            return this->name == other.name && this->price == other.price && this->discount == other.discount &&
                   this->tax == other.tax && this->netPrice == other.netPrice;
        }
    };
    auto storage =
        make_storage({},
                     make_table("products",
                                make_column("name", &Product::name),
                                make_column("price", &Product::price),
                                make_column("discount", &Product::discount),
                                make_column("tax", &Product::tax),
                                make_column("net_price",
                                            &Product::netPrice,
                                            generated_always_as(c(&Product::price) * (1 - c(&Product::discount)) *
                                                                (1 + c(&Product::tax))))));
    storage.sync_schema();
    Product product{"ABC Widget", 100, 0.05, 0.07, -100};
    SECTION("insert") {
        storage.insert(product);
    }
    SECTION("replace") {
        storage.replace(product);
    }

    auto allProducts = storage.get_all<Product>();
    decltype(allProducts) expectedProducts;
    expectedProducts.push_back({"ABC Widget", 100, 0.05, 0.07, 101.65});
    REQUIRE(allProducts == expectedProducts);
}
#endif

TEST_CASE("last insert rowid") {
    struct Object {
        int id;
    };

    auto storage = make_storage("", make_table("objects", make_column("id", &Object::id, primary_key())));

    storage.sync_schema();

    SECTION("ordinary insert") {
        int id = storage.insert<Object>({0});
        REQUIRE(id == storage.last_insert_rowid());
        REQUIRE_NOTHROW(storage.get<Object>(id));
    }
    SECTION("explicit insert") {
        int id = storage.insert<Object>({2}, columns(&Object::id));
        REQUIRE(id == 2);
        REQUIRE(id == storage.last_insert_rowid());
    }
    SECTION("range, prepared") {
        std::vector<Object> rng{{0}};
        auto stmt = storage.prepare(insert_range(rng.begin(), rng.end()));
        int64 id = storage.execute(stmt);
        REQUIRE(id == storage.last_insert_rowid());
        REQUIRE_NOTHROW(storage.get<Object>(id));
    }
}

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
TEST_CASE("With clause") {
    using Catch::Matchers::Equals;

    SECTION("select") {
        using cnt = decltype(1_ctealias);
        auto storage = make_storage("");
        SECTION("with ordinary") {
            auto rows = storage.with(cte<cnt>().as(select(1)), select(column<cnt>(1_colalias)));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1}));
        }
        SECTION("with ordinary, compound") {
            auto rows = storage.with(cte<cnt>().as(select(1)),
                                     union_all(select(column<cnt>(1_colalias)), select(column<cnt>(1_colalias))));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1, 1}));
        }
        SECTION("with not enforced recursive") {
            auto rows = storage.with_recursive(cte<cnt>().as(select(1)), select(column<cnt>(1_colalias)));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1}));
        }
        SECTION("with not enforced recursive, compound") {
            auto rows =
                storage.with_recursive(cte<cnt>().as(select(1)),
                                       union_all(select(column<cnt>(1_colalias)), select(column<cnt>(1_colalias))));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1, 1}));
        }
        SECTION("with ordinary, multiple") {
            auto rows = storage.with(std::make_tuple(cte<cnt>().as(select(1))), select(column<cnt>(1_colalias)));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1}));
        }
        SECTION("with ordinary, multiple, compound") {
            auto rows = storage.with(std::make_tuple(cte<cnt>().as(select(1))),
                                     union_all(select(column<cnt>(1_colalias)), select(column<cnt>(1_colalias))));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1, 1}));
        }
        SECTION("with not enforced recursive, multiple") {
            auto rows =
                storage.with_recursive(std::make_tuple(cte<cnt>().as(select(1))), select(column<cnt>(1_colalias)));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1}));
        }
        SECTION("with not enforced recursive, multiple, compound") {
            auto rows =
                storage.with_recursive(std::make_tuple(cte<cnt>().as(select(1))),
                                       union_all(select(column<cnt>(1_colalias)), select(column<cnt>(1_colalias))));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1, 1}));
        }
        SECTION("with optional recursive") {
            auto rows = storage.with(
                cte<cnt>().as(
                    union_all(select(1), select(column<cnt>(1_colalias) + 1, where(column<cnt>(1_colalias) < 2)))),
                select(column<cnt>(1_colalias)));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1, 2}));
        }
        SECTION("with recursive") {
            auto rows = storage.with_recursive(
                cte<cnt>().as(
                    union_all(select(1), select(column<cnt>(1_colalias) + 1, where(column<cnt>(1_colalias) < 2)))),
                select(column<cnt>(1_colalias)));
            REQUIRE_THAT(rows, Equals(std::vector<int>{1, 2}));
        }
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("crud") {
        struct Object {
            int id;
        };

        auto storage = make_storage("", make_table("objects", make_column("id", &Object::id, primary_key())));

        storage.sync_schema();

        constexpr orm_cte_moniker auto data = "data"_cte;
        constexpr auto cteExpression = cte<data>().as(union_all(select(2), select(3)));

        storage.with(cteExpression, insert(into<Object>(), columns(&Object::id), select(data->*1_colalias)));
        REQUIRE(3 == storage.last_insert_rowid());

        storage.with(cteExpression, replace(into<Object>(), columns(&Object::id), select(data->*1_colalias)));
        REQUIRE(storage.changes() == 2);

        storage.with(
            cteExpression,
            update_all(
                set(c(&Object::id) = select(data->*1_colalias, from<data>(), where(data->*1_colalias == &Object::id))),
                where(c(&Object::id).in(select(data->*1_colalias)))));
        REQUIRE(storage.changes() == 2);

        storage.with(cteExpression, remove_all<Object>(where(c(&Object::id).in(select(data->*1_colalias)))));
        REQUIRE(storage.changes() == 2);
    }
#endif
}
#endif
