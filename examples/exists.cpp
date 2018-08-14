/**
 *  Example is implemented from here https://www.w3resource.com/sqlite/exists-operator.php
 */
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <iostream>

using std::cout;
using std::endl;

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

int main(int argc, char **argv) {
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
    
    storage.replace(Agent{"A007", "Ramasundar", "Bangalore", 0.15, "077-25814763"});
    storage.replace(Agent{"A003", "Alex", "London", 0.13, "075-12458969"});
    storage.replace(Agent{"A008", "Alford", "New York", 0.12, "044-25874365"});
    storage.replace(Agent{"A011", "Ravi Kumar", "Bangalore", 0.15, "077-45625874"});
    storage.replace(Agent{"A010", "Santakumar", "Chennai", 0.14, "007-22388644"});
    storage.replace(Agent{"A012", "Lucida", "San Jose", 0.12, "044-52981425"});
    storage.replace(Agent{"A005", "Anderson", "Brisban", 0.13, "045-21447739"});
    storage.replace(Agent{"A001", "Subbarao", "Bangalore", 0.14, "077-12346674"});
    storage.replace(Agent{"A002", "Mukesh", "Mumbai", 0.11, "029-12358964"});
    storage.replace(Agent{"A006", "McDen", "London", 0.15, "078-22255588"});
    storage.replace(Agent{"A004", "Ivan", "Torento", 0.15, "008-22544166"});
    storage.replace(Agent{"A009", "Benjamin", "Hampshair", 0.11, "008-22536178"});
    
    storage.replace(Customer{"C00013", "Holmes", "London", "London", "UK", 2, 6000.00, 5000.00, 7000.00, 4000.00, "BBBBBBB", "A003"});
    storage.replace(Customer{"C00001", "Micheal", "New York", "New York", "USA", 2, 3000.00, 5000.00, 2000.00, 6000.00, "CCCCCCC", "A008"});
    storage.replace(Customer{"C00020", "Albert", "New York", "New York", "USA", 3, 5000.00, 7000.00, 6000.00, 6000.00, "BBBBSBB", "A008"});
    storage.replace(Customer{"C00025", "Ravindran", "Bangalore", "Bangalore", "India", 2, 5000.00, 7000.00, 4000.00, 8000.00, "AVAVAVA", "A011"});
    storage.replace(Customer{"C00024", "Cook", "London", "London", "UK", 2, 4000.00, 9000.00, 7000.00, 6000.00, "FSDDSDF", "A006"});
    storage.replace(Customer{"C00015", "Stuart", "London", "London", "UK", 1, 6000.00, 8000.00, 3000.00, 11000.00, "GFSGERS", "A003"});
    storage.replace(Customer{"C00002", "Bolt", "New York", "New York", "USA", 3, 5000.00, 7000.00, 9000.00, 3000.00, "DDNRDRH", "A008"});
    storage.replace(Customer{"C00018", "Fleming", "Brisban", "Brisban", "Australia", 2, 7000.00, 7000.00, 9000.00, 5000.00, "NHBGVFC", "A005"});
    storage.replace(Customer{"C00021", "Jacks", "Brisban", "Brisban", "Australia", 1, 7000.00, 7000.00, 7000.00, 7000.00, "WERTGDF", "A005"});
    storage.replace(Customer{"C00019", "Yearannaidu", "Chennai", "Chennai", "India", 1, 8000.00, 7000.00, 7000.00, 8000.00, "ZZZZBFV", "A010"});
    storage.replace(Customer{"C00005", "Sasikant", "Mumbai", "Mumbai", "India", 1, 7000.00, 11000.00, 7000.00, 11000.00, "147-25896312", "A002"});
    storage.replace(Customer{"C00007", "Ramanathan", "Chennai", "Chennai", "India", 1, 7000.00, 11000.00, 9000.00, 9000.00, "GHRDWSD", "A010"});
    storage.replace(Customer{"C00022", "Avinash", "Mumbai", "Mumbai", "India", 2, 7000.00, 11000.00, 9000.00, 9000.00, "113-12345678", "A002"});
    storage.replace(Customer{"C00004", "Winston", "Brisban", "Brisban", "Australia", 1, 5000.00, 8000.00, 7000.00, 6000.00, "AAAAAAA", "A005"});
    storage.replace(Customer{"C00023", "Karl", "London", "London", "UK", 0, 4000.00, 6000.00, 7000.00, 3000.00, "AAAABAA", "A006"});
    storage.replace(Customer{"C00006", "Shilton", "Torento", "Torento", "Canada", 1, 10000.00, 7000.00, 6000.00, 11000.00, "DDDDDDD", "A004"});
    storage.replace(Customer{"C00010", "Charles", "Hampshair", "Hampshair", "UK", 3, 6000.00, 4000.00, 5000.00, 5000.00, "MMMMMMM", "A009"});
    storage.replace(Customer{"C00017", "Srinivas", "Bangalore", "Bangalore", "India", 2, 8000.00, 4000.00, 3000.00, 9000.00, "AAAAAAB", "A007"});
    storage.replace(Customer{"C00012", "Steven", "San Jose", "San Jose", "USA", 1, 5000.00, 7000.00, 9000.00, 3000.00, "KRFYGJK", "A012"});
    storage.replace(Customer{"C00008", "Karolina", "Torento", "Torento", "Canada", 1, 7000.00, 7000.00, 9000.00, 5000.00, "HJKORED", "A004"});
    storage.replace(Customer{"C00003", "Martin", "Torento", "Torento", "Canada", 2, 8000.00, 7000.00, 7000.00, 8000.00, "MJYURFD", "A004"});
    storage.replace(Customer{"C00009", "Ramesh", "Mumbai", "Mumbai", "India", 3, 8000.00, 7000.00, 3000.00, 12000.00, "Phone No", "A002"});
    storage.replace(Customer{"C00014", "Rangarappa", "Bangalore", "Bangalore", "India", 2, 8000.00, 11000.00, 7000.00, 12000.00, "AAAATGF", "A001"});
    storage.replace(Customer{"C00016", "Venkatpati", "Bangalore", "Bangalore", "India", 2, 8000.00, 11000.00, 7000.00, 12000.00, "JRTVFDD", "A007"});
    storage.replace(Customer{"C00011", "Sundariya", "Chennai", "Chennai", "India", 3, 7000.00, 11000.00, 7000.00, 11000.00, "PPHGRTS", "A010"});
    
    {
        //  SELECT agent_code,agent_name,working_area,commission
        //  FROM agents
        //  WHERE exists
        //      (SELECT *
        //      FROM customer
        //      WHERE grade=3 AND agents.agent_code=customer.agent_code)
        //      ORDER BY commission;
        auto rows = storage.select(columns(&Agent::code, &Agent::name, &Agent::workingArea, &Agent::comission),
                                   where(exists(select(asterisk<Customer>(),
                                                       where(is_equal(&Customer::grade, 3) and is_equal(&Agent::code, &Customer::agentCode))))),
                                   order_by(&Agent::comission));
        cout << "AGENT_CODE  AGENT_NAME                                WORKING_AREA  COMMISSION" << endl;
        cout << "----------  ----------------------------------------  ------------  ----------" << endl;
        for(auto &row : rows) {
            cout << std::get<0>(row) << '\t' <<  std::get<1>(row) << '\t' << std::get<2>(row) << '\t' << std::get<3>(row) << endl;
        }
    }
    {
        //  SELECT cust_code, cust_name, cust_city, grade
        //  FROM customer
        //  WHERE grade=2 AND EXISTS
        //      (SELECT COUNT(*)
        //      FROM customer
        //      WHERE grade=2
        //      GROUP BY grade
        //      HAVING COUNT(*)>2);
        auto rows = storage.select(columns(&Customer::code, &Customer::name, &Customer::city, &Customer::grade),
                                   where(is_equal(&Customer::grade, 2)
                                         and exists(select(count<Customer>(),
                                                           where(is_equal(&Customer::grade, 2)),
                                                           group_by(&Customer::grade),
                                                           having(greater_than(count(), 2))))));
        cout << "CUST_CODE   CUST_NAME   CUST_CITY                            GRADE" << endl;
        cout << "----------  ----------  -----------------------------------  ----------" << endl;
        for(auto &row : rows) {
            cout << std::get<0>(row) << '\t' <<  std::get<1>(row) << '\t' << std::get<2>(row) << '\t' << std::get<3>(row) << endl;
        }
        
    }
    
    return 0;
}
