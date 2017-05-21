//
//  CoreFunctions.cpp
//  CPPTest
//
//  Created by John Zakharov on 15.04.17.
//  Copyright © 2017 John Zakharov. All rights reserved.
//

#include "sqlite_orm.h"

#include <iostream>
#include <cassert>

using std::cout;
using std::endl;

struct MarvelHero {
    int id;
    std::string name;
    std::string abilities;
    short points;
};

int main(int argc, char **argv) {
    cout << "path = " << argv[0] << endl;
    
    using namespace sqlite_orm;
    auto storage = make_storage("core_functions.sqlite",
                                make_table("marvel",
                                           make_column("id",
                                                       &MarvelHero::id,
                                                       primary_key()),
                                           make_column("name",
                                                       &MarvelHero::name),
                                           make_column("abilities",
                                                       &MarvelHero::abilities),
                                           make_column("points",
                                                       &MarvelHero::points)));
    storage.sync_schema();
    
    storage.remove_all<MarvelHero>();
    
    //  insert values..
    storage.insert(MarvelHero{ -1, "Tony Stark", "Iron man, playboy, billionaire, philanthropist", 5 });
    storage.insert(MarvelHero{ -1, "Thor", "Storm god", -10 });
    storage.insert(MarvelHero{ -1, "Vision", "Min Stone", 4 });
    storage.insert(MarvelHero{ -1, "Captain America", "Vibranium shield", -3 });
    storage.insert(MarvelHero{ -1, "Hulk", "Strength", -15 });
    storage.insert(MarvelHero{ -1, "Star Lord", "Humor", 19 });
    storage.insert(MarvelHero{ -1, "Peter Parker", "Spiderman", 16 });
    storage.insert(MarvelHero{ -1, "Clint Barton", "Hawkeye", -11 });
    storage.insert(MarvelHero{ -1, "Natasha Romanoff", "Black widow", 8 });
    storage.insert(MarvelHero{ -1, "Groot", "I am Groot!", 2 });
    
    //  SELECT LENGTH(name) FROM marvel
    auto nameLengths = storage.select(length(&MarvelHero::name));   //  nameLengths is std::vector<int>
    cout << "nameLengths.size = " << nameLengths.size() << endl;
    for(auto &len : nameLengths) {
        cout << len << " ";
    }
    cout << endl;
    
    //  SELECT name, LENGTH(name) FROM marvel
    auto namesWithLengths = storage.select(columns(&MarvelHero::name, length(&MarvelHero::name)));  //  namesWithLengths is std::vector<std::tuple<std::string, int>>
    cout << "namesWithLengths.size = " << namesWithLengths.size() << endl;
    for(auto &row : namesWithLengths) {
        cout << "LENGTH(" << std::get<0>(row) << ") = " << std::get<1>(row) << endl;
    }
    
    //  SELECT name FROM marvel WHERE LENGTH(name) > 5
    auto namesWithLengthGreaterThan5 = storage.select(&MarvelHero::name, where(greater_than(length(&MarvelHero::name), 5)));    //  std::vector<std::string>
    cout << "namesWithLengthGreaterThan5.size = " << namesWithLengthGreaterThan5.size() << endl;
    for(auto &name : namesWithLengthGreaterThan5) {
        cout << "name = " << name << endl;
    }
    
    //  SELECT LENGTH('ototo')
    auto custom = storage.select(length("ototo"));
    cout << "custom = " << custom.front() << endl;
    
    //  SELECT LENGTH(1990), LENGTH(CURRENT_TIMESTAMP)
    auto customTwo = storage.select(columns(length(1990), length(storage.current_timestamp())));
    cout << "customTwo = {" << std::get<0>(customTwo.front()) << ", " << std::get<1>(customTwo.front()) << "}" << endl;
    
    //  SELECT ABS(points) FROM marvel
    auto absPoints = storage.select(abs(&MarvelHero::points));  //  std::vector<std::shared_ptr<int>>
    cout << "absPoints: ";
    for(auto &value : absPoints) {
        if(value) {
            cout << *value;
        }else{
            cout << "null";
        }
        cout << " ";
    }
    cout << endl;
    
    //  SELECT name FROM marvel WHERE ABS(points) < 5
    auto namesByAbs = storage.select(&MarvelHero::name, where(lesser_than(abs(&MarvelHero::points), 5)));
    cout << "namesByAbs.size = " << namesByAbs.size() << endl;
    for(auto &name : namesByAbs) {
        cout << name << endl;
    }
    cout << endl;
    
    //  SELECT length(abs(points)) FROM marvel
    auto twoFunctions = storage.select(length(abs(&MarvelHero::points)));
    cout << "twoFunctions.size = " << twoFunctions.size() << endl;
    cout << endl;
    
    //  SELECT LOWER(name) FROM marvel
    auto lowerNames = storage.select(lower(&MarvelHero::name));
    cout << "lowerNames.size = " << lowerNames.size() << endl;
    for(auto &name : lowerNames) {
        cout << name << endl;
    }
    cout << endl;
    
    //  SELECT UPPER(abilities) FROM marvel
    auto upperAbilities = storage.select(upper(&MarvelHero::abilities));
    cout << "upperAbilities.size = " << upperAbilities.size() << endl;
    for(auto &abilities : upperAbilities) {
        cout << abilities << endl;
    }
    cout << endl;
    
    storage.transaction([&]{
        storage.remove_all<MarvelHero>();
        
        //  SELECT changes()
        auto rowsRemoved = storage.select(changes()).front();
        cout << "rowsRemoved = " << rowsRemoved << endl;
        assert(rowsRemoved == storage.changes());
        return false;
    });
    
    /*//  SELECT CHAR(67,72,65,82)
    auto charString = storage.select(char_(67,72,65,82)).front();
    cout << "SELECT CHAR(67,72,65,82) = *" << charString << "*" << endl;*/
    
    //  SELECT LOWER(name) || '@marvel.com' FROM marvel
    /*auto emails = storage.select(conc(lower(&MarvelHero::name), "@marvel.com"));
    cout << "emails.size = " << emails.size() << endl;
    for(auto &email : emails) {
        cout << email << endl;
    }
    cout << endl;*/
    
    return 0;
}
