#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("index named table") {
    using std::string;
    struct OrderTable {
        string order_number;
        string start_time;
        string end_time;
        int type;
        int renewal_times;
        int status;
        float amount;
        string openid;
        string nickname;
        string parking_space_number;
        string license_plate_number;
        string parking_code;
        string phone_number;
        string appointment_time;
        string arrival_time;
        string leave_parking_lot_time;
        string parking_time;
        string leave_parking_space_time;
        string in_barrier_gate_number;
        string out_barrier_gate_number;
        int parking_duration;
        string create_time;
    };

    struct pay_info {
        int id;

        int pay_type;
        int payment;
        float amount;
        string pay_time;
        std::unique_ptr<string> order_number;
    };

    auto storage =
        make_storage("mysqlite.sqlite",
                     make_table("order",
                                make_column("order_number", &OrderTable::order_number, primary_key()),
                                make_column("start_time", &OrderTable::start_time),
                                make_column("end_time", &OrderTable::end_time),
                                make_column("type", &OrderTable::type),
                                make_column("renewal_times", &OrderTable::renewal_times),
                                make_column("status", &OrderTable::status),
                                make_column("amount", &OrderTable::amount),
                                make_column("openid", &OrderTable::openid),
                                make_column("nickname", &OrderTable::nickname),
                                make_column("parking_space_number", &OrderTable::parking_space_number),
                                make_column("license_plate_number", &OrderTable::license_plate_number),
                                make_column("parking_code", &OrderTable::parking_code),
                                make_column("phone_number", &OrderTable::phone_number),
                                make_column("appointment_time", &OrderTable::appointment_time),
                                make_column("arrival_time", &OrderTable::arrival_time),
                                make_column("leave_parking_lot_time", &OrderTable::leave_parking_lot_time),
                                make_column("parking_time", &OrderTable::parking_time),
                                make_column("leave_parking_space_time", &OrderTable::leave_parking_space_time),
                                make_column("in_barrier_gate_number", &OrderTable::in_barrier_gate_number),
                                make_column("out_barrier_gate_number", &OrderTable::out_barrier_gate_number),
                                make_column("parking_duration", &OrderTable::parking_duration),
                                make_column("create_time", &OrderTable::create_time)),

                     make_table("pay_info",
                                make_column("id", &pay_info::id, primary_key()),

                                make_column("pay_type", &pay_info::pay_type),
                                make_column("payment", &pay_info::payment),
                                make_column("amount", &pay_info::amount),
                                make_column("pay_time", &pay_info::pay_time),
                                make_column("order_number", &pay_info::order_number),
                                foreign_key(&pay_info::order_number).references(&OrderTable::order_number)));
    storage.sync_schema();
}
