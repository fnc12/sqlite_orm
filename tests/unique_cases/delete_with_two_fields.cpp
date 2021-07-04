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

    SECTION("none") {
        storage.remove_all<Device>(where(in(std::make_tuple(&Device::serialNumber, &Device::deviceId),
                                            values(std::make_tuple("abc", "123"), std::make_tuple("def", "456")))));

        storage.remove_all<Device>(
            where(in(std::make_tuple(&Device::serialNumber, &Device::deviceId),
                     values(std::vector<std::tuple<std::string, std::string>>{std::make_tuple("abc", "123"),
                                                                              std::make_tuple("def", "456")}))));
        REQUIRE(storage.count<Device>() == 0);
    }
    SECTION("two devices") {
        Device device_abc{"abc", "123"};
        Device device_def{"def", "456"};

        storage.replace(device_abc);
        REQUIRE(storage.count<Device>() == 1);
        storage.replace(device_def);
        REQUIRE(storage.count<Device>() == 2);

        SECTION("static condition") {
            storage.remove_all<Device>(where(in(std::make_tuple(&Device::serialNumber, &Device::deviceId),
                                                values(std::make_tuple("abc", "123"), std::make_tuple("def", "456")))));
        }
        SECTION("dynamic condition") {
            storage.remove_all<Device>(
                where(in(std::make_tuple(&Device::serialNumber, &Device::deviceId),
                         values(std::vector<std::tuple<std::string, std::string>>{std::make_tuple("abc", "123"),
                                                                                  std::make_tuple("def", "456")}))));
        }
        REQUIRE(storage.count<Device>() == 0);
    }
}
