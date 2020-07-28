#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("delete with two fields") {
    struct Device {
        std::string serialNumber;
        std::string deviceId;
    };
    auto storage = make_storage({},
                                make_table("devices",
                                           make_column("serial_number", &Device::serialNumber),
                                           make_column("device_id", &Device::deviceId)));
    storage.sync_schema();
    storage.remove_all<Device>(where(in(std::make_tuple(&Device::serialNumber, &Device::deviceId),
                                        values(std::make_tuple("abc", "123"), std::make_tuple("def", "456")))));

    storage.remove_all<Device>(
        where(in(std::make_tuple(&Device::serialNumber, &Device::deviceId),
                 values(std::vector<std::tuple<std::string, std::string>>{std::make_tuple("abc", "123"),
                                                                          std::make_tuple("def", "456")}))));
}
