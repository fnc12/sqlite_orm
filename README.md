# SQLite ORM
SQLite ORM light header only library for modern C++

# Advantages

* **Intuitive syntax**
* **Built with modern C++14 features (no macros)**
* **Follows single responsibility principle** - no need write code inside you data model classes
* **Easy integration** - single header only lib.
* **The only dependency - libsqlite3**
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

auto storage = sqlite_orm::make_storage("db.sqlite",
                                        sqlite_orm::make_table("users",
                                                               sqlite_orm::make_column("id",
                                                                                       &User::id,
                                                                                       sqlite_orm::autoincrement(),
                                                                                       sqlite_orm::not_null(),
                                                                                       sqlite_orm::primary_key()),
                                                               sqlite_orm::make_column("first_name",
                                                                                       &User::firstName,
                                                                                       sqlite_orm::not_null()),
                                                               sqlite_orm::make_column("last_name",
                                                                                       &User::lastName,
                                                                                       sqlite_orm::not_null()),
                                                               sqlite_orm::make_column("birth_date",
                                                                                       &User::birthDate,
                                                                                       sqlite_orm::not_null()),
                                                               sqlite_orm::make_column("image_url",
                                                                                       &User::imageUrl,
                                                                                       sqlite_orm::not_null()),
                                                               sqlite_orm::make_column("type_id",
                                                                                       &User::typeId,
                                                                                       sqlite_orm::not_null())));
```

Too easy isn't it? You do not have to specify mapped type expllicitly - it is deduced from your member pointers you pass during making a column. To create a column you have to pass two arguments at least: its name in the table and your mapped class member pointer. You can also add extra arguments to tell your storage about schema options like `not_null`, `primary_key` or `autoincrement` (order isn't important).

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

# Installation

Just put `src/sqlite_orm.h` into you folder with headers. Also it is recommended to keep project libraries' sources in separate folders cause there is no normal dependency manager for C++ yet.
