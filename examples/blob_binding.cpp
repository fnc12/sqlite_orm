#include <sqlite_orm/sqlite_orm.h>
#include <iostream>

using namespace sqlite_orm;
using std::cout;
using std::endl;

/**
 *  This is a struct which we want to store as BLOB.
 *  Format is simple: every int field will be stored as
 *  4 byte value one by one. 16 bytes total.
 */
struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
    Rect() = default;
    Rect(int x, int y, int width, int height) : x{x}, y{y}, width{width}, height{height} {}
#endif
};

bool operator==(const Rect& lhs, const Rect& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width && lhs.height == rhs.height;
}

struct Zone {
    int id = 0;
    Rect rect;  //  this member will be mapped as BLOB column

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
    Zone() = default;
    Zone(int id, Rect rect) : id{id}, rect{rect} {}
#endif
};

bool operator==(const Zone& lhs, const Zone& rhs) {
    return lhs.id == rhs.id && lhs.rect == rhs.rect;
}

namespace sqlite_orm {

    /**
     *  First of all is a type_printer template class.
     *  It is responsible for sqlite type string representation.
     *  We want Rect to be `BLOB` so let's just derive from
     *  blob_printer. Also there are other printers: real_printer,
     *  integer_printer and text_printer.
     */
    template<>
    struct type_printer<Rect> : public blob_printer {};

    /**
     *  This is a binder class. It is used to bind c++ values to sqlite queries.
     *  Here we have to create rect binary representation and bind it as std::vector<char>.
     *  Any statement_binder specialization must have `int bind(sqlite3_stmt*, int, const T&)` function
     *  which returns bind result. Also you can call any of `sqlite3_bind_*` functions directly.
     *  More here https://www.sqlite.org/c3ref/bind_blob.html
     */
    template<>
    struct statement_binder<Rect> {

        int bind(sqlite3_stmt* stmt, int index, const Rect& value) {
            std::vector<char> blobValue;
            blobValue.reserve(16);
            auto encodeInteger = [&blobValue](int value) {
                auto preciseValue = int32_t(value);
                const auto intPointer = &preciseValue;
                auto charPointer = (const char*)(intPointer);
                blobValue.push_back(charPointer[0]);
                blobValue.push_back(charPointer[1]);
                blobValue.push_back(charPointer[2]);
                blobValue.push_back(charPointer[3]);
            };
            encodeInteger(value.x);
            encodeInteger(value.y);
            encodeInteger(value.width);
            encodeInteger(value.height);
            return statement_binder<std::vector<char>>().bind(stmt, index, blobValue);
        }
    };

    /**
     *  field_printer is used in `dump` and `where` functions. Here we have to create
     *  a string from mapped object.
     */
    template<>
    struct field_printer<Rect> {
        std::string operator()(const Rect& value) const {
            std::stringstream ss;
            ss << "{ x = " << value.x << ", y = " << value.y << ", width = " << value.width
               << ", height = " << value.height << " }";
            return ss.str();
        }
    };

    /**
     *  This is a reverse operation: here we have to specify a way to transform std::vector<char> received from
     *  database to our Rect object. Every `row_extractor` specialization must have `extract(sqlite3_stmt *stmt,
     * 	int columnIndex)` function which returns a mapped type value.
     */
    template<>
    struct row_extractor<Rect> {

        Rect extract(sqlite3_stmt* stmt, int columnIndex) {
            auto blobPointer = sqlite3_column_blob(stmt, columnIndex);
            auto charPointer = (const char*)blobPointer;
            Rect value;
            auto decodeInteger = [charPointer](int& integer, int index) {
                auto pointerWithOffset = charPointer + index * 4;
                auto intPointer = (const int32_t*)pointerWithOffset;
                integer = int(*intPointer);
            };
            decodeInteger(value.x, 0);
            decodeInteger(value.y, 1);
            decodeInteger(value.width, 2);
            decodeInteger(value.height, 3);
            return value;
        }
    };
}

int main() {
    auto storage = make_storage(
        {},
        make_table("zones", make_column("id", &Zone::id, primary_key()), make_column("rect", &Zone::rect)));
    storage.sync_schema();
    storage.replace(Zone{1, Rect{10, 10, 200, 300}});

    auto allZones = storage.get_all<Zone>();
    cout << "zones count = " << allZones.size() << ":" << endl;  //  zones count = 1:
    for(auto& zone: allZones) {
        cout << "zone = " << storage.dump(zone)
             << endl;  //  zone = { id : '1', rect : '{ x = 10, y = 10, width = 200, height = 300 }' }
    }

    storage.update_all(set(c(&Zone::rect) = Rect{20, 20, 500, 600}), where(c(&Zone::id) == 1));
    cout << endl;
    allZones = storage.get_all<Zone>();
    cout << "zones count = " << allZones.size() << ":" << endl;  //  zones count = 1:
    for(auto& zone: allZones) {
        cout << "zone = " << storage.dump(zone)
             << endl;  //  zone = { id : '1', rect : '{ x = 20, y = 20, width = 500, height = 600 }' }
    }

    return 0;
}
