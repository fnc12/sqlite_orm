//
//  self_join.cpp
//  CPPTest
//
//  Created by John Zakharov on 09.02.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#include "sqlite_orm.h"

struct Employee {
    int employeeId;
    std::string lastName;
    std::string firstName;
    std::string title;
    int reportsTo;
    std::string birthDate;
    std::string hireDate;
    std::string address;
    std::string city;
    std::string state;
    std::string country;
    std::string postalCode;
    std::string phone;
    std::string fax;
    std::string email;
};

int main() {
    using namespace sqlite_orm;
    auto storage = make_storage("self_join.sqlite",
                                make_table("employees",
                                           make_column("EmployeeId",
                                                       &Employee::employeeId,
                                                       autoincrement(),
                                                       primary_key()),
                                           make_column("LastName",
                                                       &Employee::lastName),
                                           make_column("FirstName",
                                                       &Employee::firstName),
                                           make_column("Title",
                                                       &Employee::title),
                                           make_column("ReportsTo",
                                                       &Employee::reportsTo),
                                           make_column("BirthDate",
                                                       &Employee::birthDate),
                                           make_column("HireDate",
                                                       &Employee::hireDate),
                                           make_column("Address",
                                                       &Employee::address),
                                           make_column("City",
                                                       &Employee::city),
                                           make_column("State",
                                                       &Employee::state),
                                           make_column("Country",
                                                       &Employee::country),
                                           make_column("PostalCode",
                                                       &Employee::postalCode),
                                           make_column("Phone",
                                                       &Employee::phone),
                                           make_column("Fax",
                                                       &Employee::fax),
                                           make_column("Email",
                                                       &Employee::email),
                                           foreign_key(&Employee::reportsTo).references(&Employee::employeeId)));
    storage.sync_schema();
    
    return 0;
}
