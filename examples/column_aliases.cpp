#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <cassert>

using std::cout;
using std::endl;
using std::system_error;
using namespace sqlite_orm;

void marvel_hero_ordered_by_o_pos() {
    struct MarvelHero {
        int id = 0;
        std::string name;
        std::string abilities;
        short points = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        MarvelHero() {}
        MarvelHero(int id, std::string name, std::string abilities, short points) :
            id{id}, name{std::move(name)}, abilities{std::move(abilities)}, points{points} {}
#endif
    };

    auto storage = make_storage("",
                                make_table("marvel",
                                           make_column("id", &MarvelHero::id, primary_key()),
                                           make_column("name", &MarvelHero::name),
                                           make_column("abilities", &MarvelHero::abilities),
                                           make_column("points", &MarvelHero::points)));
    storage.sync_schema();

    //  insert values
    storage.transaction([&storage] {
        storage.insert(MarvelHero{-1, "Tony Stark", "Iron man, playboy, billionaire, philanthropist", 5});
        storage.insert(MarvelHero{-1, "Thor", "Storm god", -10});
        storage.insert(MarvelHero{-1, "Vision", "Min Stone", 4});
        storage.insert(MarvelHero{-1, "Captain America", "Vibranium shield", -3});
        storage.insert(MarvelHero{-1, "Hulk", "Strength", -15});
        storage.insert(MarvelHero{-1, "Star Lord", "Humor", 19});
        storage.insert(MarvelHero{-1, "Peter Parker", "Spiderman", 16});
        storage.insert(MarvelHero{-1, "Clint Barton", "Hawkeye", -11});
        storage.insert(MarvelHero{-1, "Natasha Romanoff", "Black widow", 8});
        storage.insert(MarvelHero{-1, "Groot", "I am Groot!", 2});

        return true;
    });

    {
        //  SELECT name, instr(abilities, 'o') i
        //  FROM marvel
        //  WHERE i > 0
        //  ORDER BY i
        auto rows = storage.select(columns(&MarvelHero::name, as<colalias_i>(instr(&MarvelHero::abilities, "o"))),
                                   where(greater_than(get<colalias_i>(), 0)),
                                   order_by(get<colalias_i>()));
        for (auto& row: rows) {
            cout << get<0>(row) << '\t' << get<1>(row) << '\n';
        }
    }
    cout << endl;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    {
        constexpr orm_column_alias auto i = "i"_col;
        //  SELECT name, instr(abilities, 'o') i
        //  FROM marvel
        //  WHERE i > 0
        //  ORDER BY i
        auto rows = storage.select(columns(&MarvelHero::name, as<i>(instr(&MarvelHero::abilities, "o"))),
                                   where(i > 0),
                                   order_by(i));
        for (auto& row: rows) {
            cout << get<0>(row) << '\t' << get<1>(row) << '\n';
        }
    }
    cout << endl;
#endif
    {
        //  SELECT name, instr(abilities, 'o')
        //  FROM marvel
        //  ORDER BY 2
        auto rows = storage.select(columns(&MarvelHero::name, instr(&MarvelHero::abilities, "o")), order_by(2));
        for (auto& row: rows) {
            cout << get<0>(row) << '\t' << get<1>(row) << '\n';
        }
    }
    cout << endl;
}

void cpp20_column_pointer() {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    struct Result {
        int64 id;
        time_t stamp;
    };

    struct LastResult : Result {};
    constexpr orm_table_reference auto last_result = c<LastResult>();

    auto storage = make_storage(
        "",
        make_table("result", make_column("id", &Result::id, primary_key()), make_column("stamp", &Result::stamp)),
        make_table<LastResult>("last_result",
                               make_column("id", &LastResult::id, primary_key()),
                               make_column("stamp", &LastResult::stamp)));
    storage.sync_schema();

    //  SELECT "last_result"."id", "last_result"."stamp" FROM "last_result"
    std::string sql = storage.dump(select(columns(last_result->*&LastResult::id, last_result->*&LastResult::stamp)));
    cout << sql << endl;
#endif
}

int main() {
    try {
        marvel_hero_ordered_by_o_pos();
        cpp20_column_pointer();
    } catch (const system_error& e) {
        cout << "[" << e.code() << "] " << e.what();
    }

    return 0;
}
