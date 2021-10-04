
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <memory>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

struct Entry {
    int id;
    std::string uniqueColumn;
    std::unique_ptr<std::string> nullableColumn;
};

int main(int, char**) {
    using namespace sqlite_orm;
    auto storage = make_storage("unique.sqlite",
                                make_table("unique_test",
                                           make_column("id", &Entry::id, autoincrement(), primary_key()),
                                           make_column("unique_text", &Entry::uniqueColumn, unique()),
                                           make_column("nullable_text", &Entry::nullableColumn)));
    storage.sync_schema();
    storage.remove_all<Entry>();

    try {
        auto sameString = "Bebe Rexha";

        auto id1 = storage.insert(Entry{0, sameString, std::make_unique<std::string>("The way I are")});
        cout << "inserted " << storage.dump(storage.get<Entry>(id1)) << endl;

        //  it's ok but the next line will throw std::system_error

        auto id2 = storage.insert(Entry{0, sameString, std::make_unique<std::string>("I got you")});
        cout << "inserted " << storage.dump(storage.get<Entry>(id2)) << endl;
    } catch(const std::system_error& e) {
        cerr << e.what() << endl;
    }

    return 0;
}
