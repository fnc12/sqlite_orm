/**
 *  Example is implemented from here https://www.w3resource.com/sqlite/exists-operator.php
 */
#include <sqlite_orm/sqlite_orm.h>
#include <string>

struct Customer {
    std::string code;
    std::string name;
    std::string city;
    std::string workingArea;
    std::string country;
    int grade;
    double openingAmt;
    double receiveAmt;
    double paymentAmt;
    double outstandingAmt;
    std::string phoneNo;
    std::string agentCode;
};

struct Agent {
    std::string code;
    std::string name;
    std::string workingArea;
    double comission;
    std::string phoneNo;
    std::string country;
};

int main() {
    using namespace sqlite_orm;
    
    auto storage = make_storage("exists.sqlite",
                                make_table("customer",
                                           make_column("CUST_CODE", &Customer::code, primary_key()),
                                           make_column("CUST_NAME", &Customer::name),
                                           make_column("CUST_CITY", &Customer::city),
                                           make_column("WORKING_AREA", &Customer::workingArea),
                                           make_column("CUST_COUNTRY", &Customer::country),
                                           make_column("GRADE", &Customer::grade),
                                           make_column("OPENING_AMT", &Customer::openingAmt),
                                           make_column("RECEIVE_AMT", &Customer::receiveAmt),
                                           make_column("PAYMENT_AMT", &Customer::paymentAmt),
                                           make_column("OUTSTANDING_AMT", &Customer::outstandingAmt),
                                           make_column("PHONE_NO", &Customer::phoneNo),
                                           make_column("AGENT_CODE", &Customer::agentCode)),
                                make_table("agents",
                                           make_column("AGENT_CODE", &Agent::code, primary_key()),
                                           make_column("AGENT_NAME", &Agent::name),
                                           make_column("WORKING_AREA", &Agent::workingArea),
                                           make_column("COMMISSION", &Agent::comission),
                                           make_column("PHONE_NO", &Agent::phoneNo),
                                           make_column("COUNTRY", &Agent::country)));
    storage.sync_schema();
    storage.remove_all<Agent>();
    storage.remove_all<Customer>();
    
    storage.replace(Customer{"C00013", "Holmes", "London", "London", "UK", 2, 6000.00, 5000.00, 7000.00, 4000.00, "BBBBBBB", "A003"});
    storage.replace(Customer{"C00001", "Micheal", "New York", "New York", "USA", 2, 3000.00, 5000.00, 2000.00, 6000.00, "CCCCCCC", "A008"});
    storage.replace(Customer{"C00020", "Albert", "New York", "New York", "USA", 3, 5000.00, 7000.00, 6000.00, 6000.00, "BBBBSBB", "A008"});
    storage.replace(Customer{"C00025", "Ravindran", "Bangalore", "Bangalore", "India", 2, 5000.00, 7000.00, 4000.00, 8000.00, "AVAVAVA", "A011"});
    storage.replace(Customer{"C00024", "Cook", "London", "London", "UK", 2, 4000.00, 9000.00, 7000.00, 6000.00, "FSDDSDF", "A006"});
//    storage.replace(Customer{"C00015", "Stuart", "London", "London",})
    
    return 0;
}
