#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <cstdint>

using namespace sqlite_orm;
using internal::count_tuple, internal::is_pkcol_implicitly_insertable;

namespace {
    struct CustomRowIdKeyType {
        std::uint32_t low;
        std::int32_t high;
    };
}
template<>
struct sqlite_orm::type_printer<CustomRowIdKeyType> : public integer_printer {};

TEST_CASE("is_pkcol_implicitly_insertable") {
    struct User {
        int intId;
        int64 int64Id;
        long long longlongId;  // note: distinct from `int64` depending on platform
        bool boolId;
        CustomRowIdKeyType customId;
        std::string username;
    };

    std::tuple columns(
        ///
        /// implicitly insertable
        ///
        //  works but must manually ensure data precision integrity, soon to be deprecated
        make_column("", &User::intId, primary_key()),
        //  works but deprecated
        make_column("", &User::intId, primary_key().autoincrement()),
        make_column("", &User::int64Id, primary_key()),
        make_column("", &User::int64Id, primary_key().autoincrement()),
        make_column("", &User::longlongId, primary_key()),
        make_column("", &User::longlongId, primary_key().autoincrement()),
        //  works but must manually ensure data precision integrity, soon to be deprecated
        make_column("", &User::boolId, primary_key()),
        //  works but deprecated
        make_column("", &User::boolId, primary_key().autoincrement()),
        make_column("", &User::customId, primary_key()),
        make_column("", &User::customId, primary_key().autoincrement()),
        make_column("", &User::username, primary_key(), default_value("Clint Eastwood")),
        make_column("", &User::username, primary_key(), default_value(std::vector<int>{})),
        ///
        /// not implicitly insertable
        ///
        make_column("", &User::username, primary_key()));

    STATIC_REQUIRE(count_tuple<decltype(columns), is_pkcol_implicitly_insertable>::value == 12);
}