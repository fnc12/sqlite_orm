
#include <sqlite_orm/sqlite_orm.h>
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

struct Contact {
    int id = 0;
    std::string firstName;
    std::string lastName;
    std::string phone;
};

int main(int, char** argv) {
    cout << "path = " << argv[0] << endl;

    using namespace sqlite_orm;
    auto storage = make_storage("core_functions.sqlite",
                                make_table("marvel",
                                           make_column("id", &MarvelHero::id, primary_key()),
                                           make_column("name", &MarvelHero::name),
                                           make_column("abilities", &MarvelHero::abilities),
                                           make_column("points", &MarvelHero::points)),
                                make_table("contacts",
                                           make_column("contact_id", &Contact::id, primary_key()),
                                           make_column("first_name", &Contact::firstName),
                                           make_column("last_name", &Contact::lastName),
                                           make_column("phone", &Contact::phone)));
    storage.sync_schema();

    storage.remove_all<MarvelHero>();

    //  insert values..
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

        cout << "SELECT last_insert_rowid() = " << storage.select(last_insert_rowid()).front() << endl;
        return true;
    });

    Contact john{0, "John", "Doe", "410-555-0168"};
    Contact lily{0, "Lily", "Bush", "410-444-9862"};
    john.id = storage.insert(john);
    lily.id = storage.insert(lily);

    //  SELECT LENGTH(name)
    //  FROM marvel
    auto nameLengths = storage.select(length(&MarvelHero::name));  //  nameLengths is std::vector<int>
    cout << "nameLengths.size = " << nameLengths.size() << endl;
    for(auto& len: nameLengths) {
        cout << len << " ";
    }
    cout << endl;

    //  SELECT name, LENGTH(name)
    //  FROM marvel
    auto namesWithLengths = storage.select(
        columns(&MarvelHero::name,
                length(&MarvelHero::name)));  //  namesWithLengths is std::vector<std::tuple<std::string, int>>
    cout << "namesWithLengths.size = " << namesWithLengths.size() << endl;
    for(auto& row: namesWithLengths) {
        cout << "LENGTH(" << std::get<0>(row) << ") = " << std::get<1>(row) << endl;
    }

    //  SELECT name
    //  FROM marvel
    //  WHERE LENGTH(name) > 5
    auto namesWithLengthGreaterThan5 =
        storage.select(&MarvelHero::name, where(length(&MarvelHero::name) > 5));  //  std::vector<std::string>
    cout << "namesWithLengthGreaterThan5.size = " << namesWithLengthGreaterThan5.size() << endl;
    for(auto& name: namesWithLengthGreaterThan5) {
        cout << "name = " << name << endl;
    }

    //  SELECT LENGTH('ototo')
    auto custom = storage.select(length("ototo"));
    cout << "custom = " << custom.front() << endl;

    //  SELECT LENGTH(1990), LENGTH(CURRENT_TIMESTAMP)
    auto customTwo = storage.select(columns(length(1990), length(storage.current_timestamp())));
    cout << "customTwo = {" << std::get<0>(customTwo.front()) << ", " << std::get<1>(customTwo.front()) << "}" << endl;

    //  SELECT ABS(points)
    //  FROM marvel
    auto absPoints = storage.select(abs(&MarvelHero::points));  //  std::vector<std::unique_ptr<int>>
    cout << "absPoints: ";
    for(auto& value: absPoints) {
        if(value) {
            cout << *value;
        } else {
            cout << "null";
        }
        cout << " ";
    }
    cout << endl;

    //  SELECT name
    //  FROM marvel
    //  WHERE ABS(points) < 5
    auto namesByAbs = storage.select(&MarvelHero::name, where(abs(&MarvelHero::points) < 5));
    cout << "namesByAbs.size = " << namesByAbs.size() << endl;
    for(auto& name: namesByAbs) {
        cout << name << endl;
    }
    cout << endl;

    //  SELECT length(abs(points))
    //  FROM marvel
    auto twoFunctions = storage.select(length(abs(&MarvelHero::points)));
    cout << "twoFunctions.size = " << twoFunctions.size() << endl;
    cout << endl;

    //  SELECT LOWER(name)
    //  FROM marvel
    auto lowerNames = storage.select(lower(&MarvelHero::name));
    cout << "lowerNames.size = " << lowerNames.size() << endl;
    for(auto& name: lowerNames) {
        cout << name << endl;
    }
    cout << endl;

    //  SELECT UPPER(abilities)
    //  FROM marvel
    auto upperAbilities = storage.select(upper(&MarvelHero::abilities));
    cout << "upperAbilities.size = " << upperAbilities.size() << endl;
    for(auto& abilities: upperAbilities) {
        cout << abilities << endl;
    }
    cout << endl;

    storage.transaction([&] {
        storage.remove_all<MarvelHero>();

        //  SELECT changes()
        {
            auto rowsRemoved = storage.select(changes()).front();
            cout << "rowsRemoved = " << rowsRemoved << endl;
            assert(rowsRemoved == storage.changes());
        }

        //  SELECT total_changes()
        {
            auto rowsRemoved = storage.select(total_changes()).front();
            cout << "rowsRemoved = " << rowsRemoved << endl;
            assert(rowsRemoved == storage.changes());
        }
        return false;
    });

#if SQLITE_VERSION_NUMBER >= 3007016

    //  SELECT CHAR(67, 72, 65, 82)
    auto charString = storage.select(char_(67, 72, 65, 82)).front();
    cout << "SELECT CHAR(67,72,65,82) = *" << charString << "*" << endl;

    //  SELECT LOWER(name) || '@marvel.com'
    //  FROM marvel
    auto emails = storage.select(lower(&MarvelHero::name) || c("@marvel.com"));
    cout << "emails.size = " << emails.size() << endl;
    for(auto& email: emails) {
        cout << email << endl;
    }
    cout << endl;

#endif

    //  TRIM examples are taken from here https://www.techonthenet.com/sqlite/functions/trim.php

    //  SELECT TRIM('   TechOnTheNet.com   ')
    cout << "trim '   TechOnTheNet.com   ' = '" << storage.select(trim("   TechOnTheNet.com   ")).front() << "'"
         << endl;

    //  SELECT TRIM('000123000', '0')
    cout << "TRIM('000123000', '0') = " << storage.select(trim("000123000", "0")).front() << endl;

    //  SELECT TRIM('zTOTNxyxzyyy', 'xyz')
    cout << "SELECT TRIM('zTOTNxyxzyyy', 'xyz') = " << storage.select(trim("zTOTNxyxzyyy", "xyz")).front() << endl;

    //  SELECT TRIM('42totn6372', '0123456789')
    cout << "TRIM('42totn6372', '0123456789') = " << storage.select(trim("42totn6372", "0123456789")).front() << endl;

    //  SELECT RANDOM()
    for(auto i = 0; i < 10; ++i) {
        cout << "RANDOM() = " << storage.select(sqlite_orm::random()).front() << endl;
    }

    //  SELECT * FROM marvel ORDER BY RANDOM()
    for(auto& hero: storage.iterate<MarvelHero>(order_by(sqlite_orm::random()))) {
        cout << "hero = " << storage.dump(hero) << endl;
    }

    //  https://www.techonthenet.com/sqlite/functions/ltrim.php

    //  SELECT ltrim('   TechOnTheNet.com');
    cout << "ltrim('   TechOnTheNet.com') = *" << storage.select(ltrim("   TechOnTheNet.com")).front() << "*" << endl;

    //  SELECT ltrim('   TechOnTheNet.com   ');
    cout << "ltrim('   TechOnTheNet.com   ') = *" << storage.select(ltrim("   TechOnTheNet.com   ")).front() << "*"
         << endl;

    //  SELECT ltrim('   TechOnTheNet.com    is great!');
    cout << "ltrim('   TechOnTheNet.com    is great!') = *"
         << storage.select(ltrim("   TechOnTheNet.com    is great!")).front() << "*" << endl;

    {  //  core functions can be use within prepared statements as well!

        auto lTrimStatement = storage.prepare(select(ltrim("000123", "0")));

        //  SELECT ltrim('000123', '0');
        cout << "ltrim('000123', '0') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('123123totn', '123');
        get<0>(lTrimStatement) = "123123totn";
        get<1>(lTrimStatement) = "123";
        cout << "ltrim('123123totn', '123') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('123123totn123', '123');
        get<0>(lTrimStatement) = "123123totn123";
        get<1>(lTrimStatement) = "123";
        cout << "ltrim('123123totn123', '123') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('xyxzyyyTOTN', 'xyz');
        get<0>(lTrimStatement) = "xyxzyyyTOTN";
        get<1>(lTrimStatement) = "xyz";
        cout << "ltrim('xyxzyyyTOTN', 'xyz') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('6372totn', '0123456789');
        get<0>(lTrimStatement) = "6372totn";
        get<1>(lTrimStatement) = "0123456789";
        cout << "ltrim('6372totn', '0123456789') = " << storage.execute(lTrimStatement).front() << endl;
    }

    //  https://www.techonthenet.com/sqlite/functions/rtrim.php

    //  SELECT rtrim('TechOnTheNet.com   ');
    cout << "rtrim('TechOnTheNet.com   ') = *" << storage.select(rtrim("TechOnTheNet.com   ")).front() << "*" << endl;

    //  SELECT rtrim('   TechOnTheNet.com   ');
    cout << "rtrim('   TechOnTheNet.com   ') = *" << storage.select(rtrim("   TechOnTheNet.com   ")).front() << "*"
         << endl;

    //  SELECT rtrim('TechOnTheNet.com    is great!   ');
    cout << "rtrim('TechOnTheNet.com    is great!   ') = *"
         << storage.select(rtrim("TechOnTheNet.com    is great!   ")).front() << "*" << endl;

    //  SELECT rtrim('123000', '0');
    cout << "rtrim('123000', '0') = *" << storage.select(rtrim("123000", "0")).front() << "*" << endl;

    //  SELECT rtrim('totn123123', '123');
    cout << "rtrim('totn123123', '123') = *" << storage.select(rtrim("totn123123", "123")).front() << "*" << endl;

    //  SELECT rtrim('123totn123123', '123');
    cout << "rtrim('123totn123123', '123') = *" << storage.select(rtrim("123totn123123", "123")).front() << "*" << endl;

    //  SELECT rtrim('TOTNxyxzyyy', 'xyz');
    cout << "rtrim('TOTNxyxzyyy', 'xyz') = *" << storage.select(rtrim("TOTNxyxzyyy", "xyz")).front() << "*" << endl;

    //  SELECT rtrim('totn6372', '0123456789');
    cout << "rtrim('totn6372', '0123456789') = *" << storage.select(rtrim("totn6372", "0123456789")).front() << "*"
         << endl;

    //  SELECT coalesce(10,20);
    cout << "coalesce(10,20) = " << storage.select(coalesce<int>(10, 20)).front() << endl;

    //  SELECT substr('SQLite substr', 8);
    cout << "substr('SQLite substr', 8) = " << storage.select(substr("SQLite substr", 8)).front() << endl;

    //  SELECT substr('SQLite substr', 1, 6);
    cout << "substr('SQLite substr', 1, 6) = " << storage.select(substr("SQLite substr", 1, 6)).front() << endl;

    //  SELECT hex(67);
    cout << "hex(67) = " << storage.select(hex(67)).front() << endl;

    //  SELECT quote('hi')
    cout << "SELECT quote('hi') = " << storage.select(quote("hi")).front() << endl;

    //  SELECT hex(randomblob(10))
    cout << "SELECT hex(randomblob(10)) = " << storage.select(hex(randomblob(10))).front() << endl;

    //  SELECT instr('something about it', 't')
    cout << "SELECT instr('something about it', 't') = " << storage.select(instr("something about it", "t")).front()
         << endl;

    {
        cout << endl;
        struct o_pos : alias_tag {
            static const std::string& get() {
                static const std::string res = "o_pos";
                return res;
            }
        };

        //  SELECT name, instr(abilities, 'o') o_pos
        //  FROM marvel
        //  WHERE o_pos > 0
        auto rows = storage.select(columns(&MarvelHero::name, as<o_pos>(instr(&MarvelHero::abilities, "o"))),
                                   where(greater_than(get<o_pos>(), 0)));
        for(auto& row: rows) {
            cout << get<0>(row) << '\t' << get<1>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT replace('AA B CC AAA','A','Z')
    cout << "SELECT replace('AA B CC AAA','A','Z') = " << storage.select(replace("AA B CC AAA", "A", "Z")).front()
         << endl;

    //  SELECT replace('This is a cat','This','That')
    cout << "SELECT replace('This is a cat','This','That') = "
         << storage.select(replace("This is a cat", "This", "That")).front() << endl;

    //  UPDATE contacts
    //  SET phone = REPLACE(phone, '410', '+1-410')
    storage.update_all(set(c(&Contact::phone) = replace(&Contact::phone, "410", "+1-410")));
    cout << "Contacts:" << endl;
    for(auto& contact: storage.iterate<Contact>()) {
        cout << storage.dump(contact) << endl;
    }

    //  SELECT round(1929.236, 2)
    cout << "SELECT round(1929.236, 2) = " << storage.select(round(1929.236, 2)).front() << endl;

    //  SELECT round(1929.236, 1)
    cout << "SELECT round(1929.236, 1) = " << storage.select(round(1929.236, 1)).front() << endl;

    //  SELECT round(1929.236)
    cout << "SELECT round(1929.236) = " << storage.select(round(1929.236)).front() << endl;

    //  SELECT round(0.5)
    cout << "SELECT round(0.5) = " << storage.select(round(0.5)).front() << endl;
#ifdef SQLITE_SOUNDEX
    //  SELECT soundex('Schn Thomson')
    cout << "SELECT soundex('Schn Thomson') = " << storage.select(soundex("Schn Thomson")).front() << endl;
#endif

    //  SELECT unicode('A')
    cout << "SELECT unicode('A') = " << storage.select(unicode("A")).front() << endl;

    //  SELECT unicode('Brush')
    cout << "SELECT unicode('Brush') = " << storage.select(unicode("Brush")).front() << endl;

    //  SELECT unicode(substr('Brush', 2))
    cout << "SELECT unicode(substr('Brush', 2)) = " << storage.select(unicode(substr("Brush", 2))).front() << endl;

    //  SELECT typeof(1)
    cout << "SELECT typeof(1) = " << storage.select(typeof_(1)).front() << endl;

    return 0;
}
