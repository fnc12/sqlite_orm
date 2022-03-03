#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Issue 86") {
    struct Data {
        uint8_t mUsed = 0;
        int mId = 0;
        int mCountryId = 0;
        int mLangId = 0;
        std::string mName;
    };

    auto storage = make_storage("",
                                make_table("cities",
                                           make_column("U", &Data::mUsed),
                                           make_column("Id", &Data::mId),
                                           make_column("CntId", &Data::mCountryId),
                                           make_column("LId", &Data::mLangId),
                                           make_column("N", &Data::mName)));
    storage.sync_schema();
    std::string CurrCity = "ototo";
    auto vms = storage.select(columns(&Data::mId, &Data::mLangId), where(like(&Data::mName, CurrCity)));
}
