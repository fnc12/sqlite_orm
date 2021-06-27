#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace GetAllWithTwoTablesInternal {
    struct Pattern {
        std::string value;
    };
    struct Item {
        int id = 0;
        std::string attributes;
    };

    inline bool operator==(const Item& lhs, const Item& rhs) {
        return lhs.id == rhs.id && lhs.attributes == rhs.attributes;
    }
}

TEST_CASE("get_all with two tables") {
    using Catch::Matchers::UnorderedEquals;
    using namespace GetAllWithTwoTablesInternal;

    auto storage = make_storage(
        {},
        make_table("patterns", make_column("pattern", &Pattern::value)),
        make_table("items", make_column("id", &Item::id, primary_key()), make_column("attributes", &Item::attributes)));
    storage.sync_schema();

    const Item item1{1, "one"};
    const Item item2{2, "two"};
    const Item item3{3, "three"};
    const Item item4{4, "nwa"};

    storage.replace(item1);
    storage.replace(item2);
    storage.replace(item3);

    SECTION("straight insert") {
        std::vector<Pattern> patterns;
        patterns.push_back({"n"});
        patterns.push_back({"w"});

        storage.begin_transaction();
        storage.insert_range(patterns.begin(), patterns.end());
    }
    SECTION("pointers insert") {
        std::vector<std::unique_ptr<Pattern>> patterns;
        patterns.push_back(std::make_unique<Pattern>(Pattern{"n"}));
        patterns.push_back(std::make_unique<Pattern>(Pattern{"w"}));

        storage.begin_transaction();
        storage.insert_range<Pattern>(patterns.begin(),
                                      patterns.end(),
                                      [](const std::unique_ptr<Pattern>& pointer) -> const Pattern& {
                                          return *pointer;
                                      });
    }
    {
        auto rows = storage.select(&Item::id, where(like(&Item::attributes, conc(conc("%", &Pattern::value), "%"))));
        REQUIRE_THAT(rows, UnorderedEquals<int>({1, 2}));

        auto items = storage.get_all<Item>(where(
            in(&Item::id, select(&Item::id, where(like(&Item::attributes, conc(conc("%", &Pattern::value), "%")))))));
        REQUIRE_THAT(items, UnorderedEquals<Item>({item1, item2}));
    }
    {
        storage.replace(Item{4, "nwa"});
        auto rows = storage.select(&Item::id,
                                   where(like(&Item::attributes, conc(conc("%", &Pattern::value), "%"))),
                                   group_by(&Item::id),
                                   having(is_equal(count(&Pattern::value), select(count<Pattern>()))));
        REQUIRE_THAT(rows, UnorderedEquals<int>({4}));

        auto items = storage.get_all<Item>(
            where(in(&Item::id,
                     select(&Item::id,
                            where(like(&Item::attributes, conc(conc("%", &Pattern::value), "%"))),
                            group_by(&Item::id),
                            having(is_equal(count(&Pattern::value), select(count<Pattern>())))))));
        REQUIRE_THAT(items, UnorderedEquals<Item>({item4}));
    }
    storage.rollback();
}
