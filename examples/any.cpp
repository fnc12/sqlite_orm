/**
 *  This example demonstrates how to use std::any as a mapped type.
 *  It is a bit more complex than the other examples, because it
 *  uses custom row_extractor and statement_binder.
 *  Please note that this implementation is not the one and only
 *  option of implementation of `std:any` binding to sqlite_orm.
 *  It is just an example of how to use std::any as a mapped type.
 *  Implementation is based on 5 types SQLite supports:
 *  NULL, INTEGER, REAL, TEXT, BLOB.
 *  NULL is mapped to std::nullopt.
 *  INTEGER is mapped to int.
 *  REAL is mapped to double.
 *  TEXT is mapped to std::string.
 *  BLOB is mapped to std::vector<char>.
*/
#include <sqlite_orm/sqlite_orm.h>
#include <any>
#include <iostream>
#include <cstdio>  //  std::remove

using namespace sqlite_orm;
using std::cout;
using std::endl;

namespace sqlite_orm {

    template<>
    struct row_extractor<std::any> {
        std::any extract(sqlite3_stmt* stmt, int columnIndex) const {
            const int type = sqlite3_column_type(stmt, columnIndex);
            switch (type) {
                case SQLITE_NULL:
                    return std::nullopt;
                case SQLITE_INTEGER:
                    return sqlite3_column_int(stmt, columnIndex);
                case SQLITE_FLOAT:
                    return sqlite3_column_double(stmt, columnIndex);
                case SQLITE_TEXT: {
                    const unsigned char* text = sqlite3_column_text(stmt, columnIndex);
                    return std::string(reinterpret_cast<const char*>(text));
                }
                case SQLITE_BLOB: {
                    const void* blob = sqlite3_column_blob(stmt, columnIndex);
                    const int size = sqlite3_column_bytes(stmt, columnIndex);
                    return std::vector<char>(reinterpret_cast<const char*>(blob),
                                             reinterpret_cast<const char*>(blob) + size);
                }
                default:
                    throw std::runtime_error("Unsupported SQLite column type for std::any");
            }
        }

        std::any extract(sqlite3_value* value) const {
            const int type = sqlite3_value_type(value);
            switch (type) {
                case SQLITE_NULL:
                    return std::nullopt;
                case SQLITE_INTEGER:
                    return sqlite3_value_int(value);
                case SQLITE_FLOAT:
                    return sqlite3_value_double(value);
                case SQLITE_TEXT: {
                    const unsigned char* text = sqlite3_value_text(value);
                    return std::string(reinterpret_cast<const char*>(text));
                }
                case SQLITE_BLOB: {
                    const void* blob = sqlite3_value_blob(value);
                    const int size = sqlite3_value_bytes(value);
                    return std::vector<char>(reinterpret_cast<const char*>(blob),
                                             reinterpret_cast<const char*>(blob) + size);  // Handle BLOB
                }
                default:
                    throw std::runtime_error("Unsupported SQLite value type for std::any");
            }
        }
    };

    template<>
    struct statement_binder<std::any> {
        int bind(sqlite3_stmt* stmt, int index, const std::any& value) const {
            if (!value.has_value()) {
                return sqlite3_bind_null(stmt, index);
            }

            if (value.type() == typeid(int)) {
                return sqlite3_bind_int(stmt, index, std::any_cast<int>(value));
            } else if (value.type() == typeid(double)) {
                return sqlite3_bind_double(stmt, index, std::any_cast<double>(value));
            } else if (value.type() == typeid(std::string)) {
                const auto& text = std::any_cast<std::string>(value);
                return sqlite3_bind_text(stmt, index, text.c_str(), static_cast<int>(text.size()), SQLITE_TRANSIENT);
            } else if (value.type() == typeid(std::vector<char>)) {
                const auto& blob = std::any_cast<std::vector<char>>(value);
                return sqlite3_bind_blob(stmt, index, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
            }

            return SQLITE_MISMATCH;
        }
    };

    template<>
    struct type_printer<std::any> : public text_printer {};

    template<>
    struct field_printer<std::any> {
        std::string operator()(const std::any& value) const {
            if (!value.has_value()) {
                return "NULL";
            }

            if (value.type() == typeid(int)) {
                return std::to_string(std::any_cast<int>(value));
            } else if (value.type() == typeid(double)) {
                return std::to_string(std::any_cast<double>(value));
            } else if (value.type() == typeid(std::string)) {
                return std::any_cast<std::string>(value);
            } else if (value.type() == typeid(std::vector<char>)) {
                const auto& blob = std::any_cast<std::vector<char>>(value);
                std::ostringstream oss;
                oss << "0x";
                for (unsigned char c: blob) {
                    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
                }
                return oss.str();
            }

            throw std::runtime_error("Unsupported type in std::any field_printer");
        }
    };
}

int main() {
    struct Value {
        int id = 0;
        std::any value;
    };
    auto filename = "any.sqlite";
    std::remove(filename);
    auto storage = make_storage(
        filename,
        make_table("test", make_column("id", &Value::id, primary_key()), make_column("value", &Value::value)));
    storage.sync_schema();
    storage.replace(Value{1, std::any{1}});
    storage.replace(Value{2, std::any{2.5}});
    storage.replace(Value{3, std::any{std::string("Hello, world!")}});
    storage.replace(
        Value{4, std::any{std::vector<char>{'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!'}}});

    cout << "Test:" << endl;
    for (auto& test: storage.iterate<Value>()) {
        cout << storage.dump(test) << endl;
    }
    cout << endl;
    return 0;
}
