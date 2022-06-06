#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#include <memory>  // std::default_delete
#include <stdlib.h>  // free()

using namespace sqlite_orm;
using std::default_delete;
using std::unique_ptr;

TEST_CASE("Empty storage") {
    auto storage = make_storage("empty.sqlite");
    storage.table_exists("table");
}

TEST_CASE("Remove") {
    struct Object {
        int id;
        std::string name;
    };

    {
        auto storage = make_storage(
            "test_remove.sqlite",
            make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));

        storage.sync_schema();
        storage.remove_all<Object>();

        auto id1 = storage.insert(Object{0, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(id1);
        REQUIRE(storage.count<Object>() == 0);
    }
    {
        auto storage = make_storage("test_remove.sqlite",
                                    make_table("objects",
                                               make_column("id", &Object::id),
                                               make_column("name", &Object::name),
                                               primary_key(&Object::id)));
        storage.sync_schema();
        storage.remove_all<Object>();

        auto id1 = storage.insert(Object{0, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(id1);
        REQUIRE(storage.count<Object>() == 0);
    }
    {
        auto storage = make_storage("",
                                    make_table("objects",
                                               make_column("id", &Object::id),
                                               make_column("name", &Object::name),
                                               primary_key(&Object::id, &Object::name)));
        storage.sync_schema();
        storage.replace(Object{1, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(1, "Skillet");
        REQUIRE(storage.count<Object>() == 0);

        storage.replace(Object{1, "Skillet"});
        storage.replace(Object{2, "Paul Cless"});
        REQUIRE(storage.count<Object>() == 2);
        storage.remove<Object>(1, "Skillet");
        REQUIRE(storage.count<Object>() == 1);
    }
}
#if SQLITE_VERSION_NUMBER >= 3031000
TEST_CASE("insert with generated column") {
    struct Product {
        std::string name;
        double price = 0;
        double discount = 0;
        double tax = 0;
        double netPrice = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Product() = default;
        Product(std::string name, double price, double discount, double tax, double netPrice) :
            name{move(name)}, price{price}, discount{discount}, tax{tax}, netPrice{netPrice} {}
#endif

        bool operator==(const Product &other) const {
            return this->name == other.name && this->price == other.price && this->discount == other.discount &&
                   this->tax == other.tax && this->netPrice == other.netPrice;
        }
    };
    auto storage =
        make_storage({},
                     make_table("products",
                                make_column("name", &Product::name),
                                make_column("price", &Product::price),
                                make_column("discount", &Product::discount),
                                make_column("tax", &Product::tax),
                                make_column("net_price",
                                            &Product::netPrice,
                                            generated_always_as(c(&Product::price) * (1 - c(&Product::discount)) *
                                                                (1 + c(&Product::tax))))));
    storage.sync_schema();
    Product product{"ABC Widget", 100, 0.05, 0.07, -100};
    SECTION("insert") {
        storage.insert(product);
    }
    SECTION("replace") {
        storage.replace(product);
    }

    auto allProducts = storage.get_all<Product>();
    decltype(allProducts) expectedProducts;
    expectedProducts.push_back({"ABC Widget", 100, 0.05, 0.07, 101.65});
    REQUIRE(allProducts == expectedProducts);
}
#endif
TEST_CASE("insert") {
    struct Object {
        int id;
        std::string name;
    };

    struct ObjectWithoutRowid {
        int id;
        std::string name;
    };

    auto storage = make_storage(
        "test_insert.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)),
        make_table("objects_without_rowid",
                   make_column("id", &ObjectWithoutRowid::id, primary_key()),
                   make_column("name", &ObjectWithoutRowid::name))
            .without_rowid());

    storage.sync_schema();
    storage.remove_all<Object>();
    storage.remove_all<ObjectWithoutRowid>();

    for(auto i = 0; i < 100; ++i) {
        storage.insert(Object{
            0,
            "Skillet",
        });
        REQUIRE(storage.count<Object>() == i + 1);
    }

    auto initList = {
        Object{
            0,
            "Insane",
        },
        Object{
            0,
            "Super",
        },
        Object{
            0,
            "Sun",
        },
    };

    auto countBefore = storage.count<Object>();
    SECTION("straight") {
        storage.insert_range(initList.begin(), initList.end());
        REQUIRE(storage.count<Object>() == countBefore + static_cast<int>(initList.size()));

        //  test empty container
        std::vector<Object> emptyVector;
        REQUIRE_NOTHROW(storage.insert_range(emptyVector.begin(), emptyVector.end()));
    }
    SECTION("pointers") {
        std::vector<unique_ptr<Object>> pointers;
        pointers.reserve(initList.size());
        std::transform(initList.begin(), initList.end(), std::back_inserter(pointers), [](const Object &object) {
            return std::make_unique<Object>(Object{object});
        });
        storage.insert_range(pointers.begin(), pointers.end(), &std::unique_ptr<Object>::operator*);

        //  test empty container
        std::vector<unique_ptr<Object>> emptyVector;
        REQUIRE_NOTHROW(
            storage.insert_range(emptyVector.begin(), emptyVector.end(), &std::unique_ptr<Object>::operator*));
    }

    //  test insert without rowid
    storage.insert(ObjectWithoutRowid{10, "Life"});
    REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
    storage.insert(ObjectWithoutRowid{20, "Death"});
    REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
}

struct SqrtFunction {
    static int callsCount;

    double operator()(double arg) const {
        ++callsCount;
        return std::sqrt(arg);
    }

    static const char *name() {
        return "SQRT_CUSTOM";
    }
};

int SqrtFunction::callsCount = 0;

struct HasPrefixFunction {
    static int callsCount;
    static int objectsCount;

    HasPrefixFunction() {
        ++objectsCount;
    }

    HasPrefixFunction(const HasPrefixFunction &) {
        ++objectsCount;
    }

    HasPrefixFunction(HasPrefixFunction &&) {
        ++objectsCount;
    }

    ~HasPrefixFunction() {
        --objectsCount;
    }

    bool operator()(const std::string &str, const std::string &prefix) {
        ++callsCount;
        return str.compare(0, prefix.size(), prefix) == 0;
    }

    static std::string name() {
        return "HAS_PREFIX";
    }
};

int HasPrefixFunction::callsCount = 0;
int HasPrefixFunction::objectsCount = 0;

struct MeanFunction {
    double total = 0;
    int count = 0;

    static int objectsCount;

    MeanFunction() {
        ++objectsCount;
    }

    MeanFunction(const MeanFunction &) {
        ++objectsCount;
    }

    MeanFunction(MeanFunction &&) {
        ++objectsCount;
    }

    ~MeanFunction() {
        --objectsCount;
    }

    void step(double value) {
        total += value;
        ++count;
    }

    double fin() const {
        return total / count;
    }

    static std::string name() {
        return "MEAN";
    }
};

int MeanFunction::objectsCount = 0;

struct FirstFunction {
    static int objectsCount;
    static int callsCount;

    FirstFunction() {
        ++objectsCount;
    }

    FirstFunction(const MeanFunction &) {
        ++objectsCount;
    }

    FirstFunction(MeanFunction &&) {
        ++objectsCount;
    }

    ~FirstFunction() {
        --objectsCount;
    }

    std::string operator()(const arg_values &args) const {
        ++callsCount;
        std::string res;
        res.reserve(args.size());
        for(auto value: args) {
            auto stringValue = value.get<std::string>();
            if(!stringValue.empty()) {
                res += stringValue.front();
            }
        }
        return res;
    }

    static const char *name() {
        return "FIRST";
    }
};

struct MultiSum {
    double sum = 0;

    static int objectsCount;

    MultiSum() {
        ++objectsCount;
    }

    MultiSum(const MeanFunction &) {
        ++objectsCount;
    }

    MultiSum(MeanFunction &&) {
        ++objectsCount;
    }

    ~MultiSum() {
        --objectsCount;
    }

    void step(const arg_values &args) {
        for(auto it = args.begin(); it != args.end(); ++it) {
            if(!it->empty() && (it->is_integer() || it->is_float())) {
                this->sum += it->get<double>();
            }
        }
    }

    double fin() const {
        return this->sum;
    }

    static const char *name() {
        return "MULTI_SUM";
    }
};

int MultiSum::objectsCount = 0;

int FirstFunction::objectsCount = 0;
int FirstFunction::callsCount = 0;

TEST_CASE("custom functions") {
    using Catch::Matchers::Contains;

    SqrtFunction::callsCount = 0;
    HasPrefixFunction::callsCount = 0;
    FirstFunction::callsCount = 0;

    std::string path;
    SECTION("in memory") {
        path = {};
    }
    SECTION("file") {
        path = "custom_function.sqlite";
        ::remove(path.c_str());
    }
    struct User {
        int id = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id) : id{id} {}
#endif
    };
    auto storage = make_storage(path, make_table("users", make_column("id", &User::id)));
    storage.sync_schema();
    //   call before creation
    REQUIRE_THROWS_WITH(storage.select(func<SqrtFunction>(4)), Contains("no such function"));

    //  create function
    REQUIRE(SqrtFunction::callsCount == 0);

    storage.create_scalar_function<SqrtFunction>();

    REQUIRE(SqrtFunction::callsCount == 0);

    //  call after creation
    {
        auto rows = storage.select(func<SqrtFunction>(4));
        REQUIRE(SqrtFunction::callsCount == 1);
        decltype(rows) expected;
        expected.push_back(2);
        REQUIRE(rows == expected);
    }

    //  create function
    REQUIRE(HasPrefixFunction::callsCount == 0);
    REQUIRE(HasPrefixFunction::objectsCount == 0);
    storage.create_scalar_function<HasPrefixFunction>();
    REQUIRE(HasPrefixFunction::callsCount == 0);
    REQUIRE(HasPrefixFunction::objectsCount == 0);

    //  call after creation
    {
        auto rows = storage.select(func<HasPrefixFunction>("one", "o"));
        decltype(rows) expected;
        expected.push_back(true);
        REQUIRE(rows == expected);
    }
    REQUIRE(HasPrefixFunction::callsCount == 1);
    REQUIRE(HasPrefixFunction::objectsCount == 0);
    {
        auto rows = storage.select(func<HasPrefixFunction>("two", "b"));
        decltype(rows) expected;
        expected.push_back(false);
        REQUIRE(rows == expected);
    }
    REQUIRE(HasPrefixFunction::callsCount == 2);
    REQUIRE(HasPrefixFunction::objectsCount == 0);

    //  delete function
    storage.delete_scalar_function<HasPrefixFunction>();

    //  delete function
    storage.delete_scalar_function<SqrtFunction>();

    storage.create_aggregate_function<MeanFunction>();

    storage.replace(User{1});
    storage.replace(User{2});
    storage.replace(User{3});
    REQUIRE(storage.count<User>() == 3);
    {
        REQUIRE(MeanFunction::objectsCount == 0);
        auto rows = storage.select(func<MeanFunction>(&User::id));
        REQUIRE(MeanFunction::objectsCount == 0);
        decltype(rows) expected;
        expected.push_back(2);
        REQUIRE(rows == expected);
    }
    storage.delete_aggregate_function<MeanFunction>();

    storage.create_scalar_function<FirstFunction>();
    {
        auto rows = storage.select(func<FirstFunction>("Vanotek", "Tinashe", "Pitbull"));
        decltype(rows) expected;
        expected.push_back("VTP");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 1);
    }
    {
        auto rows = storage.select(func<FirstFunction>("Charli XCX", "Rita Ora"));
        decltype(rows) expected;
        expected.push_back("CR");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 2);
    }
    {
        auto rows = storage.select(func<FirstFunction>("Ted"));
        decltype(rows) expected;
        expected.push_back("T");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 3);
    }
    {
        auto rows = storage.select(func<FirstFunction>());
        decltype(rows) expected;
        expected.push_back("");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 4);
    }
    storage.delete_scalar_function<FirstFunction>();

    storage.create_aggregate_function<MultiSum>();
    {
        REQUIRE(MultiSum::objectsCount == 0);
        auto rows = storage.select(func<MultiSum>(&User::id, 5));
        decltype(rows) expected;
        expected.push_back(21);
        REQUIRE(rows == expected);
        REQUIRE(MultiSum::objectsCount == 0);
    }
    storage.delete_aggregate_function<MultiSum>();
}

// Wrap std::default_delete in a function
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
template<typename T>
void delete_default(std::conditional_t<std::is_array<T>::value, std::decay_t<T>, T *> o) noexcept(
    noexcept(std::default_delete<T>{}(o))) {
    std::default_delete<T>{}(o);
}

// Integral function constant for default deletion
template<typename T>
using delete_default_t = std::integral_constant<decltype(&delete_default<T>), delete_default<T>>;
// Integral function constant variable for default deletion
template<typename T>
SQLITE_ORM_INLINE_VAR constexpr delete_default_t<T> delete_default_f{};
#endif

using free_t = std::integral_constant<decltype(&free), free>;
SQLITE_ORM_INLINE_VAR constexpr free_t free_f{};

TEST_CASE("obtain_xdestroy_for") {

    using internal::xdestroy_proxy;

    // class yielding a 'xDestroy' function pointer
    struct xdestroy_holder {
        xdestroy_fn_t xDestroy = free;

        constexpr operator xdestroy_fn_t() const noexcept {
            return xDestroy;
        }
    };

    // class yielding a function pointer not of type xdestroy_fn_t
    struct int_destroy_holder {
        using destroy_fn_t = void (*)(int *);

        destroy_fn_t destroy = nullptr;

        constexpr operator destroy_fn_t() const noexcept {
            return destroy;
        }
    };

    {
        constexpr int *int_nullptr = nullptr;
#if !defined(SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION) ||                                                             \
    (__cpp_constexpr >= 201907L)  //  Trivial default initialization in constexpr functions
        constexpr const int *const_int_nullptr = nullptr;
#endif

        // null_xdestroy_f(int*)
        constexpr xdestroy_fn_t xDestroy1 = obtain_xdestroy_for(null_xdestroy_f, int_nullptr);
        STATIC_REQUIRE(xDestroy1 == nullptr);
        REQUIRE(xDestroy1 == nullptr);

        // free(int*)
        constexpr xdestroy_fn_t xDestroy2 = obtain_xdestroy_for(free, int_nullptr);
        STATIC_REQUIRE(xDestroy2 == &free);
        REQUIRE(xDestroy2 == &free);

        // free_f(int*)
        constexpr xdestroy_fn_t xDestroy3 = obtain_xdestroy_for(free_f, int_nullptr);
        STATIC_REQUIRE(xDestroy3 == &free);
        REQUIRE(xDestroy3 == &free);

#if __cpp_constexpr >= 201603L  //  constexpr lambda
        // [](void* p){}
        constexpr auto lambda4_1 = [](void *) {};
        constexpr xdestroy_fn_t xDestroy4_1 = obtain_xdestroy_for(lambda4_1, int_nullptr);
        STATIC_REQUIRE(xDestroy4_1 == lambda4_1);
        REQUIRE(xDestroy4_1 == lambda4_1);
#else
#if !defined(_MSC_VER) || (_MSC_VER >= 1914)  //  conversion of lambda closure to function pointer using `+`
        // [](void* p){}
        auto lambda4_1 = [](void *) {};
        xdestroy_fn_t xDestroy4_1 = obtain_xdestroy_for(lambda4_1, int_nullptr);
        REQUIRE(xDestroy4_1 == lambda4_1);
#endif
#endif

        // [](int* p) { delete p; }
#if __cplusplus >= 202002L  //  default-constructible non-capturing lambdas
        constexpr auto lambda4_2 = [](int *p) {
            delete p;
        };
        using lambda4_2_t = std::remove_const_t<decltype(lambda4_2)>;
        constexpr xdestroy_fn_t xDestroy4_2 = obtain_xdestroy_for(lambda4_2, int_nullptr);
        STATIC_REQUIRE(xDestroy4_2 == &xdestroy_proxy<lambda4_2_t, int>);
        REQUIRE((xDestroy4_2 == &xdestroy_proxy<lambda4_2_t, int>));
#endif

        // default_delete<int>(int*)
        constexpr xdestroy_fn_t xDestroy5 = obtain_xdestroy_for(default_delete<int>{}, int_nullptr);
        STATIC_REQUIRE(xDestroy5 == &xdestroy_proxy<default_delete<int>, int>);
        REQUIRE((xDestroy5 == &xdestroy_proxy<default_delete<int>, int>));

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        // delete_default_f<int>(int*)
        constexpr xdestroy_fn_t xDestroy6 = obtain_xdestroy_for(delete_default_f<int>, int_nullptr);
        STATIC_REQUIRE(xDestroy6 == &xdestroy_proxy<delete_default_t<int>, int>);
        REQUIRE((xDestroy6 == &xdestroy_proxy<delete_default_t<int>, int>));

        // delete_default_f<int>(const int*)
        constexpr xdestroy_fn_t xDestroy7 = obtain_xdestroy_for(delete_default_f<int>, const_int_nullptr);
        STATIC_REQUIRE(xDestroy7 == &xdestroy_proxy<delete_default_t<int>, const int>);
        REQUIRE((xDestroy7 == &xdestroy_proxy<delete_default_t<int>, const int>));
#endif

#if __cpp_constexpr >= 201907L  //  Trivial default initialization in constexpr functions
        // xdestroy_holder{ free }(int*)
        constexpr xdestroy_fn_t xDestroy8 = obtain_xdestroy_for(xdestroy_holder{free}, int_nullptr);
        STATIC_REQUIRE(xDestroy8 == &free);
        REQUIRE(xDestroy8 == &free);

        // xdestroy_holder{ free }(const int*)
        constexpr xdestroy_fn_t xDestroy9 = obtain_xdestroy_for(xdestroy_holder{free}, const_int_nullptr);
        STATIC_REQUIRE(xDestroy9 == &free);
        REQUIRE(xDestroy9 == &free);

        // xdestroy_holder{ nullptr }(const int*)
        constexpr xdestroy_fn_t xDestroy10 = obtain_xdestroy_for(xdestroy_holder{nullptr}, const_int_nullptr);
        STATIC_REQUIRE(xDestroy10 == nullptr);
        REQUIRE(xDestroy10 == nullptr);
#endif

        // expressions that do not work
#if 0
    // can't use functions that differ from xdestroy_fn_t
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(delete_default<int>, int_nullptr);
    // can't use object yielding a function pointer that differs from xdestroy_fn_t
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(int_destroy_holder{}, int_nullptr);
    // successfully takes default_delete<void>, but default_delete statically asserts on a non-complete type `void*`
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(default_delete<void>{}, int_nullptr);
    // successfully takes default_delete<int>, but xdestroy_proxy can't call the deleter with a `const int*`
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(default_delete<int>{}, const_int_nullptr);
#endif
    }
}
