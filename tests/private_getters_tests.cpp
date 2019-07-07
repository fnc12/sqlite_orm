#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

using std::cout;
using std::endl;

TEST_CASE("Issue 343"){
    
    class A {
    public:
        A() = default;
        
        A(int id_) : id(id_) {}
        
        int getId() const {
            return this->id;
        }
        
        void setId(int id) {
            this->id = id;
        }
    private:
        int id = 0;
    };
    
    auto storage = make_storage({},
                                make_table("a",
                                           make_column("role", &A::getId, &A::setId)));
    storage.sync_schema();
    
    A object(1);
    storage.insert(object);
    
    storage.replace(object);
    
    storage.update(object);
    
    storage.remove<A>(object.getId());
    
    std::vector<A> vec{object};
    storage.insert_range(vec.begin(), vec.end());
    
    storage.replace_range(vec.begin(), vec.end());
    
    auto all = storage.get_all<A>();
    std::ignore = all;
    
}
