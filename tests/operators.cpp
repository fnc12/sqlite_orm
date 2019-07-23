#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Operators"){
    struct Object {
        std::string name;
        int nameLen;
        int number;
    };
    
    auto storage = make_storage("",
                                make_table("objects",
                                           make_column("name", &Object::name),
                                           make_column("name_len", &Object::nameLen),
                                           make_column("number", &Object::number)));
    storage.sync_schema();
    
    std::vector<std::string> names {
        "Zombie", "Eminem", "Upside down",
    };
    auto number = 10;
    for(auto &name : names) {
        storage.insert(Object{ name , int(name.length()), number });
    }
    {
        auto rows = storage.select(c(&Object::nameLen) + 1000);
        for(size_t i = 0; i < rows.size(); ++i){
            auto &row = rows[i];
            auto &name = names[i];
            REQUIRE(int(row) == name.length() + 1000);
        }
    }
    {
        auto rows = storage.select(columns(c(&Object::nameLen) + 1000));
        for(size_t i = 0; i < rows.size(); ++i){
            auto &row = rows[i];
            auto &name = names[i];
            REQUIRE(int(std::get<0>(row)) == name.length() + 1000);
        }
    }
    {
        std::string suffix = "ototo";
        auto rows = storage.select(c(&Object::name) || suffix);
        for(size_t i = 0; i < rows.size(); ++i){
            auto &row = rows[i];
            auto &name = names[i];
            REQUIRE(row == name + suffix);
        }
    }
    {
        std::string suffix = "ototo";
        auto rows = storage.select(columns(conc(&Object::name,  suffix)));
        for(size_t i = 0; i < rows.size(); ++i){
            auto &row = rows[i];
            auto &name = names[i];
            REQUIRE(std::get<0>(row) == name + suffix);
        }
    }
    {
        std::string suffix = "ototo";
        auto rows = storage.select(columns(conc(&Object::name, suffix),
                                           c(&Object::name) || suffix,
                                           &Object::name || c(suffix),
                                           c(&Object::name) || c(suffix),
                                           
                                           add(&Object::nameLen, &Object::number),
                                           c(&Object::nameLen) + &Object::number,
                                           &Object::nameLen + c(&Object::number),
                                           c(&Object::nameLen) + c(&Object::number),
                                           c(&Object::nameLen) + 1000,
                                           
                                           sub(&Object::nameLen, &Object::number),
                                           c(&Object::nameLen) - &Object::number,
                                           &Object::nameLen - c(&Object::number),
                                           c(&Object::nameLen) - c(&Object::number),
                                           c(&Object::nameLen) - 1000,
                                           
                                           mul(&Object::nameLen, &Object::number),
                                           c(&Object::nameLen) * &Object::number,
                                           &Object::nameLen * c(&Object::number),
                                           c(&Object::nameLen) * c(&Object::number),
                                           c(&Object::nameLen) * 1000,
                                           
                                           div(&Object::nameLen, &Object::number),
                                           c(&Object::nameLen) / &Object::number,
                                           &Object::nameLen / c(&Object::number),
                                           c(&Object::nameLen) / c(&Object::number),
                                           c(&Object::nameLen) / 2));
        
        for(size_t i = 0; i < rows.size(); ++i) {
            auto &row = rows[i];
            auto &name = names[i];
            REQUIRE(std::get<0>(row) == name + suffix);
            REQUIRE(std::get<1>(row) == std::get<0>(row));
            REQUIRE(std::get<2>(row) == std::get<1>(row));
            REQUIRE(std::get<3>(row) == std::get<2>(row));
            
            auto expectedAddNumber = int(name.length()) + number;
            REQUIRE(std::get<4>(row) == expectedAddNumber);
            REQUIRE(std::get<5>(row) == std::get<4>(row));
            REQUIRE(std::get<6>(row) == std::get<5>(row));
            REQUIRE(std::get<7>(row) == std::get<6>(row));
            {
                auto &rowValue = std::get<8>(row);
                REQUIRE(rowValue == int(name.length()) + 1000);
            }
            
            auto expectedSubNumber = int(name.length()) - number;
            REQUIRE(std::get<9>(row) == expectedSubNumber);
            REQUIRE(std::get<10>(row) == std::get<9>(row));
            REQUIRE(std::get<11>(row) == std::get<10>(row));
            REQUIRE(std::get<12>(row) == std::get<11>(row));
            REQUIRE(std::get<13>(row) == int(name.length()) - 1000);
            
            auto expectedMulNumber = int(name.length()) * number;
            REQUIRE(std::get<14>(row) == expectedMulNumber);
            REQUIRE(std::get<15>(row) == std::get<14>(row));
            REQUIRE(std::get<16>(row) == std::get<15>(row));
            REQUIRE(std::get<17>(row) == std::get<16>(row));
            REQUIRE(std::get<18>(row) == int(name.length()) * 1000);
            
            auto expectedDivNumber = int(name.length()) / number;
            REQUIRE(std::get<19>(row) == expectedDivNumber);
            REQUIRE(std::get<20>(row) == std::get<19>(row));
            REQUIRE(std::get<21>(row) == std::get<20>(row));
            REQUIRE(std::get<22>(row) == std::get<21>(row));
            REQUIRE(std::get<23>(row) == int(name.length()) / 2);
        }
    }
    {
        auto rows = storage.select(columns(mod(&Object::nameLen, &Object::number),
                                           c(&Object::nameLen) % &Object::number,
                                           &Object::nameLen % c(&Object::number),
                                           c(&Object::nameLen) % c(&Object::number),
                                           c(&Object::nameLen) % 5));
        for(size_t i = 0; i < rows.size(); ++i) {
            auto &row = rows[i];
            auto &name = names[i];
            REQUIRE(std::get<0>(row) == static_cast<int>(name.length()) % number);
            REQUIRE(std::get<1>(row) == std::get<0>(row));
            REQUIRE(std::get<2>(row) == std::get<1>(row));
            REQUIRE(std::get<3>(row) == std::get<2>(row));
            REQUIRE(std::get<4>(row) == static_cast<int>(name.length()) % 5);
        }
    }
}
