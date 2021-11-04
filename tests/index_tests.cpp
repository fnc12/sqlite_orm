#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("index") {
    struct User {
        int id = 0;
        std::string name;
    };

    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    {
        auto storage = make_storage({}, make_index("id_index", &User::id), table);
        storage.sync_schema();
    }
    {
        auto storage = make_storage({}, make_index("id_index", indexed_column(&User::id).asc()), table);
        storage.sync_schema();
    }
    {
        auto storage = make_storage({}, make_index("id_index", indexed_column(&User::id).desc()), table);
        storage.sync_schema();
    }
    {
        auto storage = make_storage({}, make_index("name_index", &User::name), table);
        storage.sync_schema();
    }
    {
        auto storage = make_storage({}, make_index("name_index", indexed_column(&User::name)), table);
        storage.sync_schema();
    }
    {
        auto storage = make_storage({}, make_index("name_index", indexed_column(&User::name).collate("binary")), table);
        storage.sync_schema();
    }
    {
        auto storage =
            make_storage({}, make_index("name_index", indexed_column(&User::name).collate("binary").asc()), table);
        storage.sync_schema();
    }
    {
        auto storage =
            make_storage({}, make_index("name_index", indexed_column(&User::name).collate("binary").desc()), table);
        storage.sync_schema();
    }
}

TEST_CASE("Escaped index name") {
    struct User {
        std::string group;
    };
    auto storage = make_storage("index_group.sqlite",
                                make_index("index", &User::group),
                                make_table("users", make_column("group", &User::group)));
    storage.sync_schema();
}
