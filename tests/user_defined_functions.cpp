#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

struct SqrtFunction {
    static int callsCount;

    double operator()(double arg) const {
        ++callsCount;
        return std::sqrt(arg);
    }

    static const char* name() {
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

    HasPrefixFunction(const HasPrefixFunction&) {
        ++objectsCount;
    }

    HasPrefixFunction(HasPrefixFunction&&) {
        ++objectsCount;
    }

    ~HasPrefixFunction() {
        --objectsCount;
    }

    bool operator()(const std::string& str, const std::string& prefix) {
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

    MeanFunction(const MeanFunction&) {
        ++objectsCount;
    }

    MeanFunction(MeanFunction&&) {
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

    FirstFunction(const MeanFunction&) {
        ++objectsCount;
    }

    FirstFunction(MeanFunction&&) {
        ++objectsCount;
    }

    ~FirstFunction() {
        --objectsCount;
    }

    std::string operator()(const arg_values& args) const {
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

    static const char* name() {
        return "FIRST";
    }
};

struct MultiSum {
    double sum = 0;

    static int objectsCount;

    MultiSum() {
        ++objectsCount;
    }

    MultiSum(const MeanFunction&) {
        ++objectsCount;
    }

    MultiSum(MeanFunction&&) {
        ++objectsCount;
    }

    ~MultiSum() {
        --objectsCount;
    }

    void step(const arg_values& args) {
        for(auto it = args.begin(); it != args.end(); ++it) {
            if(!it->empty() && (it->is_integer() || it->is_float())) {
                this->sum += it->get<double>();
            }
        }
    }

    double fin() const {
        return this->sum;
    }

    static const char* name() {
        return "MULTI_SUM";
    }
};

int MultiSum::objectsCount = 0;

int FirstFunction::objectsCount = 0;
int FirstFunction::callsCount = 0;

#if __cpp_aligned_new >= 201606L
struct alignas(2 * __STDCPP_DEFAULT_NEW_ALIGNMENT__) OverAlignedScalarFunction {
    int operator()(int arg) const {
        return arg;
    }

    static const char* name() {
        return "OVERALIGNED1";
    }
};

struct alignas(2 * __STDCPP_DEFAULT_NEW_ALIGNMENT__) OverAlignedAggregateFunction {
    double sum = 0;

    void step(double arg) {
        sum += arg;
    }
    double fin() const {
        return sum;
    }

    static const char* name() {
        return "OVERALIGNED2";
    }
};
#endif

struct NonDefaultCtorScalarFunction {
    const int multiplier;

    NonDefaultCtorScalarFunction(int multiplier) : multiplier{multiplier} {}

    int operator()(int arg) const {
        return multiplier * arg;
    }

    static const char* name() {
        return "CTORTEST1";
    }
};

struct NonDefaultCtorAggregateFunction {
    int sum;

    NonDefaultCtorAggregateFunction(int initialValue) : sum{initialValue} {}

    void step(int arg) {
        sum += arg;
    }
    int fin() const {
        return sum;
    }

    static const char* name() {
        return "CTORTEST2";
    }
};

TEST_CASE("custom functions") {
    using Catch::Matchers::ContainsSubstring;

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

    storage.create_aggregate_function<MeanFunction>();
    // test the case when `MeanFunction::step()` was never called
    { REQUIRE_NOTHROW(storage.select(func<MeanFunction>(&User::id))); }
    storage.delete_aggregate_function<MeanFunction>();

    //   call before creation
    REQUIRE_THROWS_WITH(storage.select(func<SqrtFunction>(4)), ContainsSubstring("no such function"));

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

#if __cpp_aligned_new >= 201606L
    {
        storage.create_scalar_function<OverAlignedScalarFunction>();
        REQUIRE_NOTHROW(storage.delete_scalar_function<OverAlignedScalarFunction>());
        storage.create_aggregate_function<OverAlignedAggregateFunction>();
        REQUIRE_NOTHROW(storage.delete_aggregate_function<OverAlignedAggregateFunction>());
    }
#endif

    storage.create_scalar_function<NonDefaultCtorScalarFunction>(42);
    {
        auto rows = storage.select(func<NonDefaultCtorScalarFunction>(1));
        decltype(rows) expected{42};
        REQUIRE(rows == expected);
    }
    storage.delete_scalar_function<NonDefaultCtorScalarFunction>();

    storage.create_aggregate_function<NonDefaultCtorAggregateFunction>(42);
    {
        auto rows = storage.select(func<NonDefaultCtorAggregateFunction>(1));
        decltype(rows) expected{43};
        REQUIRE(rows == expected);
    }
    storage.delete_aggregate_function<NonDefaultCtorAggregateFunction>();
}

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
inline int ERR_FATAL_ERROR(unsigned long errcode) {
    return errcode != 0;
}

struct noncopyable_scalar {
    int operator()(int x) const noexcept {
        return x;
    }

    constexpr noncopyable_scalar() = default;
    noncopyable_scalar(const noncopyable_scalar&) = delete;
};

struct stateful_scalar {
    int offset;

    int operator()(int x) noexcept {
        return offset += x;
    }

    constexpr stateful_scalar(int offset = 0) : offset{offset} {}
};
inline constexpr stateful_scalar offset0{};

TEST_CASE("generalized scalar udf") {
    auto storage = make_storage("");
    storage.sync_schema();

    SECTION("freestanding function") {
        constexpr auto err_fatal_error_f = "ERR_FATAL_ERROR"_scalar.quote(ERR_FATAL_ERROR);
        storage.create_scalar_function<err_fatal_error_f>();
        {
            auto rows = storage.select(err_fatal_error_f(1));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<err_fatal_error_f>();
    }

    SECTION("stateless lambda") {
        constexpr auto is_fatal_error_f = "is_fatal_error"_scalar.quote([](unsigned long errcode) {
            return errcode != 0;
        });
        storage.create_scalar_function<is_fatal_error_f>();
        {
            auto rows = storage.select(is_fatal_error_f(1));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<is_fatal_error_f>();
    }

    SECTION("function object instance") {
        constexpr auto equal_to_int_f = "equal_to"_scalar.quote(std::equal_to<int>{});
        storage.create_scalar_function<equal_to_int_f>();
        {
            auto rows = storage.select(equal_to_int_f(1, 1));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<equal_to_int_f>();
    }

    SECTION("explicit function object type") {
        constexpr auto equal_to_int_f = "equal_to"_scalar.quote<std::equal_to<int>>();
        storage.create_scalar_function<equal_to_int_f>();
        {
            auto rows = storage.select(equal_to_int_f(1, 1));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<equal_to_int_f>();
    }

    SECTION("'transparent' function object instance") {
        constexpr auto equal_to_int_f =
            "equal_to"_scalar.quote<bool(const int&, const int&) const>(std::equal_to<void>{});
        storage.create_scalar_function<equal_to_int_f>();
        {
            auto rows = storage.select(equal_to_int_f(1, 1));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<equal_to_int_f>();
    }

    SECTION("explicit 'transparent' function object type") {
        constexpr auto equal_to_int_f =
            "equal_to"_scalar.quote<bool(const int&, const int&) const, std::equal_to<void>>();
        storage.create_scalar_function<equal_to_int_f>();
        {
            auto rows = storage.select(equal_to_int_f(1, 1));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<equal_to_int_f>();
    }

    SECTION("specialized template function") {
        constexpr auto clamp_int_f = "clamp_int"_scalar.quote(std::clamp<int>);
        storage.create_scalar_function<clamp_int_f>();
        {
            auto rows = storage.select(clamp_int_f(0, 1, 1));
            decltype(rows) expected{1};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<clamp_int_f>();
    }

    SECTION("overloaded template function") {
        constexpr auto clamp_int_f =
            "clamp_int"_scalar.quote<const int&(const int&, const int&, const int&)>(std::clamp);
        storage.create_scalar_function<clamp_int_f>();
        {
            auto rows = storage.select(clamp_int_f(0, 1, 1));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<clamp_int_f>();
    }

    SECTION("non-copyable function object") {
        constexpr auto idfunc_f = "idfunc"_scalar.quote<noncopyable_scalar>();
        storage.create_scalar_function<idfunc_f>();
        {
            auto rows = storage.select(idfunc_f(1));
            decltype(rows) expected{1};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<idfunc_f>();
    }

    SECTION("stateful function object") {
        constexpr auto offset0_f = "offset0"_scalar.quote(offset0);
        storage.create_scalar_function<offset0_f>();
        {
            auto rows = storage.select(columns(offset0_f(1), offset0_f(1)));
            decltype(rows) expected{{1, 1}};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<offset0_f>();
    }
}
#endif
