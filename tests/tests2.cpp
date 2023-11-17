#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using std::unique_ptr;

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
        std::vector<unique_ptr<Object>> pointers;
        pointers.reserve(initList.size());
        std::transform(initList.begin(), initList.end(), std::back_inserter(pointers), [](const Object& object) {
            return std::make_unique<Object>(Object{object});
        });
        storage.insert_range(pointers.begin(), pointers.end(), &std::unique_ptr<Object>::operator*);

        //  test empty container
        std::vector<unique_ptr<Object>> emptyVector;
        REQUIRE_NOTHROW(
            storage.insert_range(emptyVector.begin(), emptyVector.end(), &std::unique_ptr<Object>::operator*));
    }

    //  test insert without rowid
    storage.insert(ObjectWithoutRowid{10, "Life"});
    REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
    storage.insert(ObjectWithoutRowid{20, "Death"});
    REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
}
