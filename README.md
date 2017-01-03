# SQLite ORM
SQLite ORM light header only library for modern C++

# Advantages

* **Intuitive syntax**
* **Built with modern C++14 features (no macros)**
* **Follows single responsibility principle** - no need write code inside your data model classes
* **Easy integration** - single header only lib.
* **The only dependency** - libsqlite3
* **C++ standart code style**
* **No undefined behaviour** - if something goes wrong lib throws an exeption

`sqlite_orm` library allows to create easy data model mappings to your database schema. It is built to manage (CRUD) objects with id of integer type. It also allows you to specify table names and column names explicitly no matter how your classes actually named. Take a look at example:

```c++

struct User{
    int id;
    std::string firstName;
    std::string lastName;
    int birthDate;
    std::string imageUrl;
    int typeId;
}

```

So we have database with predefined schema like `CREATE TABLE users (id integer primary key autoincrement, first_name text not null, last_name text not null, birth_date integer not null, image_url text not null, type_id integer not null)`

Now we tell `sqlite_orm` library about schema and provide database filename. We create `storage` service object that has CRUD interface. Also we create every table and every column. All code is intuitive and minimalistic.

```c++

using namespace sqlite_orm;
auto storage = make_storage("db.sqlite",
                            make_table("users",
                                       make_column("id",
                                                   &User::id,
                                                   autoincrement(),
                                                   not_null(),
                                                   primary_key()),
                                       make_column("first_name",
                                                   &User::firstName,
                                                   not_null()),
                                       make_column("last_name",
                                                   &User::lastName,
                                                   not_null()),
                                       make_column("birth_date",
                                                   &User::birthDate,
                                                   not_null()),
                                       make_column("image_url",
                                                   &User::imageUrl,
                                                   not_null()),
                                       make_column("type_id",
                                                   &User::typeId,
                                                   not_null())));
```

Too easy isn't it? You do not have to specify mapped type expllicitly - it is deduced from your member pointers you pass during making a column (for example: `&User::id`). To create a column you have to pass two arguments at least: its name in the table and your mapped class member pointer. You can also add extra arguments to tell your storage about schema options like `not_null`, `primary_key` or `autoincrement` (order isn't important).

# CRUD

Let's create and insert new `User` into database. First we need to create a `User` object with any id and call `insert` function. It will return id of just created user or throw exeption if something goes wrong.

```c++
User user{-1, "Jonh", "Doe", 664416000, "", 3 };
    
auto insertedId = storage.insert(user);
cout << "insertedId = " << insertedId << endl;
user.id = insertedId;

```

Next let's get our user by id.

```c++
try{
    auto user = storage.get<User>(insertedId);
    cout << "user = " << user.firstName << " " << user.lastName << endl;
}catch(sqlite_orm::not_found_exception) {
    cout << "user not found with id " << insertedId << endl;
}catch(...){
    cout << "unknown exeption" << endl;
}
```

Probably you may not like throwing exeptions. Me too. Exeption `not_found_exception` is thrown because return type in `get` function is not nullable. You can use alternative version `get_no_throw` which returns `std::shared_ptr` and doesn't throw `not_found_exception` if nothing found - just returns nullptr.

```c++
if(auto user = storage.get_no_throw<User>(insertedId)){
    cout << "user = " << user->firstName << " " << user->lastName << endl;
}else{
    cout << "no user with id " << insertedId << endl;
}
```

`std::shared_ptr` is used as optional in `sqlite_orm`. Of course there is class optional in C++14 located at `std::experimental::optional`. But we don't want to use it until it is `experimental`.

We can also update our user. It updates row by id provided in `user` object and sets all other non `primary_key` fields to values stored in the passed `user` object. So you can just assign members to `user` object you want and call `update`

```c++
user.firstName = "Nicholas";
user.imageUrl = "https://cdn1.iconfinder.com/data/icons/man-icon-set/100/man_icon-21-512.png"
storage.update(user);
```

And delete. To delete you have to pass id only, not whole object. Also we need to explicitly tell which class of object we want to delete. Function name is `remove` not `delete` cause `delete` is a reseved word in C++.

```c++
storage.remove<User>(insertedId)
```

Also we can extract all objects into `std::vector`.

```c++
auto allUsers = storage.get_all<User>();
cout << "all users count = " << allUsers.size() << endl;
```

# Aggregate Functions

```c++
auto averageId = storage.avg(&User::id);    //  maps to 'select avg(id) from users'
cout << "averageId = " << averageId << endl;        //  averageId = 4.5
    
auto averageBirthDate = storage.avg(&User::birthDate);  //  maps into 'select avg(birth_date) from users'
cout << "averageBirthDate = " << averageBirthDate << endl;      //  averageBirthDate = 6.64416e+08
  
auto usersCount = storage.count<User>();    //  maps to 'select count(*) from users'
cout << "users count = " << usersCount << endl;     //  users count = 8
    
auto countId = storage.count(&User::id);    //  maps to 'select count(id) from users'
cout << "countId = " << countId << endl;        //  countId = 8

auto countFirstName = storage.count(&User::firstName);   //  maps to 'select count(first_name) from users'
cout << "countFirstName = " << countFirstName << endl;      //  countFirstName = 8

auto concatedUserId = storage.group_concat(&User::id);      //  maps to 'select group_concat(id) from users'
cout << "concatedUserId = " << concatedUserId << endl;      //  concatedUserId = 1,2,3,4,5,6,7,8
    
auto concatedUserIdWithDashes = storage.group_concat(&User::id, "---");     //  maps to 'select group_concat(id, "---") from users'
cout << "concatedUserIdWithDashes = " << concatedUserIdWithDashes << endl;      //  concatedUserIdWithDashes = 1---2---3---4---5---6---7---8

if(auto maxId = storage.max(&User::id)){    //  maps to 'select max(id) from users'
    cout << "maxId = " << *maxId <<endl;    //  maxId = 12  (maxId is std::shared_ptr<int>)
}else{
    cout << "maxId is null" << endl;
}
    
if(auto maxFirstName = storage.max(&User::firstName)){  //  maps to 'select max(first_name) from users'
    cout << "maxFirstName = " << *maxFirstName << endl; //  maxFirstName = Jonh (maxFirstName is std::shared_ptr<std::string>)
}else{
    cout << "maxFirstName is null" << endl;
}

```

# Notes

To work well your data model class must be default constructable and must not have const fields mapped to database cause they are assigned during queries. Otherwise code won't compile on line with member assignment operator.

# Installation

Just put `src/sqlite_orm.h` into you folder with headers. Also it is recommended to keep project libraries' sources in separate folders cause there is no normal dependency manager for C++ yet.

# Requirements

* C++14 compatible compiler (not C++11 cause of templated lambdas in the lib).
* libsqlite3 linked to your binary
