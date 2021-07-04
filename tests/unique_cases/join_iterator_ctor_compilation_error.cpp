#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Join iterator ctor compilation error") {
    struct Tag {
        int objectId;
        std::string text;
    };

    auto storage =
        make_storage("join_error.sqlite",
                     make_table("tags", make_column("object_id", &Tag::objectId), make_column("text", &Tag::text)));
    storage.sync_schema();

    auto offs = 0;
    auto lim = 5;
    storage.select(columns(&Tag::text, count(&Tag::text)),
                   group_by(&Tag::text),
                   order_by(count(&Tag::text)).desc(),
                   limit(offs, lim));
    {
        auto statement = storage.prepare(select(columns(&Tag::text, count(&Tag::text)),
                                                group_by(&Tag::text),
                                                order_by(count(&Tag::text)).desc(),
                                                limit(offs, lim)));
        REQUIRE(get<0>(statement) == offs);
        REQUIRE(get<1>(statement) == lim);
    }
    {
        auto statement = storage.prepare(select(columns(&Tag::text, count(&Tag::text)),
                                                group_by(&Tag::text),
                                                order_by(count(&Tag::text)).desc(),
                                                limit(lim, offset(offs))));
        REQUIRE(get<0>(statement) == lim);
        REQUIRE(get<1>(statement) == offs);
    }
}
