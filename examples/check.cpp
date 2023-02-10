#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <iostream>

using namespace sqlite_orm;
using std::cout;
using std::endl;

int main() {
    struct Contact {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string email;
        std::string phone;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Contact() {}
        Contact(int id, std::string firstName, std::string lastName, std::string email, std::string phone) :
            id{id}, firstName{std::move(firstName)}, lastName{std::move(lastName)}, email{std::move(email)},
            phone{std::move(phone)} {}
#endif
    };

    struct Product {
        int id = 0;
        std::string name;
        float listPrice = 0;
        float discount = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Product() {}
        Product(int id, std::string name, float listPrice, float discount) :
            id{id}, name{std::move(name)}, listPrice{listPrice}, discount{discount} {}
#endif
    };

    auto storage = make_storage(":memory:",
                                make_table("contacts",
                                           make_column("contact_id", &Contact::id, primary_key()),
                                           make_column("first_name", &Contact::firstName),
                                           make_column("last_name", &Contact::lastName),
                                           make_column("email", &Contact::email),
                                           make_column("phone", &Contact::phone),
                                           check(length(&Contact::phone) >= 10)),
                                make_table("products",
                                           make_column("product_id", &Product::id, primary_key()),
                                           make_column("product_name", &Product::name),
                                           make_column("list_price", &Product::listPrice),
                                           make_column("discount", &Product::discount, default_value(0)),
                                           check(c(&Product::listPrice) >= &Product::discount and
                                                 c(&Product::discount) >= 0 and c(&Product::listPrice) >= 0)));
    storage.sync_schema();

    try {
        storage.insert(Contact{0, "John", "Doe", {}, "408123456"});
    } catch(const std::system_error& e) {
        cout << e.what() << endl;
    }
    storage.insert(Contact{0, "John", "Doe", {}, "(408)-123-456"});

    try {
        storage.insert(Product{0, "New Product", 900, 1000});
    } catch(const std::system_error& e) {
        cout << e.what() << endl;
    }

    try {
        storage.insert(Product{0, "New XFactor", 1000, -10});
    } catch(const std::system_error& e) {
        cout << e.what() << endl;
    }

    return 0;
}
