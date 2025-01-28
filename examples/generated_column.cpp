/**
 *  Thanks to that nice website https://database.guide/how-to-create-a-computed-column-in-sqlite/ for this example.
 */

#include <sqlite_orm/sqlite_orm.h>
#include <iostream>

#if SQLITE_VERSION_NUMBER >= 3031000
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
using namespace sqlite_orm;
using std::cout;
using std::endl;
#endif  //  ENABLE_THIS_EXAMPLE

int main() {
#ifdef ENABLE_THIS_EXAMPLE
    struct Product {
        int id = 0;
        std::string name;
        int quantity = 0;
        float price = 0;
        float totalValue = 0;
    };
    auto storage = make_storage({},
                                make_table("products",
                                           make_column("id", &Product::id, primary_key()),
                                           make_column("name", &Product::name),
                                           make_column("quantity", &Product::quantity),
                                           make_column("price", &Product::price),
                                           make_column("total_value",
                                                       &Product::totalValue,
                                                       generated_always_as(&Product::price * c(&Product::quantity)))));
    storage.sync_schema();

    storage.replace(Product{1, "Hammer", 10, 9.99f});
    storage.replace(Product{2, "Saw", 5, 11.34f});
    storage.replace(Product{3, "Wrench", 7, 37.00f});
    storage.replace(Product{4, "Chisel", 9, 23.00f});
    storage.replace(Product{5, "Bandage", 70, 120.00f});

    cout << "Products:" << endl;
    for (auto& product: storage.iterate<Product>()) {
        cout << storage.dump(product) << endl;
    }
    cout << endl;

    //  UPDATE products
    //  SET quantity = 5 WHERE id = 1;
    storage.update_all(set(c(&Product::quantity) = 5), where(c(&Product::id) == 1));

    cout << "Products after update:" << endl;
    for (auto& product: storage.iterate<Product>()) {
        cout << storage.dump(product) << endl;
    }
    cout << endl;
#endif  //  ENABLE_THIS_EXAMPLE
    return 0;
}
