#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include "catch_matchers.h"

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

struct StatelessHasPrefixFunction {
    static int callsCount;
    static int objectsCount;

    StatelessHasPrefixFunction() {
        ++objectsCount;
    }

    StatelessHasPrefixFunction(const StatelessHasPrefixFunction&) = delete;

    ~StatelessHasPrefixFunction() {
        --objectsCount;
    }

    bool operator()(const std::string& str, const std::string& prefix) {
        ++callsCount;
        return str.compare(0, prefix.size(), prefix) == 0;
    }

    static std::string name() {
        return "STATELESS_HAS_PREFIX";
    }
};

int StatelessHasPrefixFunction::callsCount = 0;
int StatelessHasPrefixFunction::objectsCount = 0;

struct HasPrefixFunction {
    static int callsCount;
    static int objectsCount;

    int& staticCallsCount;
    int& staticObjectsCount;

    HasPrefixFunction() : staticCallsCount{callsCount}, staticObjectsCount{objectsCount} {
        ++staticObjectsCount;
    }

    HasPrefixFunction(const HasPrefixFunction&) = delete;

    ~HasPrefixFunction() {
        --staticObjectsCount;
    }

    bool operator()(const std::string& str, const std::string& prefix) {
        ++staticCallsCount;
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

    static int wasInstantiatedCount;
    static int objectsCount;

    MeanFunction() {
        ++wasInstantiatedCount;
        ++objectsCount;
    }

    MeanFunction(const MeanFunction&) = delete;

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

int MeanFunction::wasInstantiatedCount = 0;
int MeanFunction::objectsCount = 0;

struct FirstFunction {
    static int objectsCount;
    static int callsCount;

    int& staticObjectsCount;
    int& staticCallsCount;

    FirstFunction() : staticObjectsCount{objectsCount}, staticCallsCount{callsCount} {
        ++staticObjectsCount;
    }

    FirstFunction(const FirstFunction&) = delete;

    ~FirstFunction() {
        --staticObjectsCount;
    }

    std::string operator()(const arg_values& args) const {
        ++staticCallsCount;
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

struct NonAllocatableAggregateFunction {
    void step(double /*arg*/) {}

    double fin() const {
        return 0;
    }

    static const char* name() {
        return "NONALLOCATABLE";
    }
};

template<>
struct std::allocator<NonAllocatableAggregateFunction> {
    using value_type = NonAllocatableAggregateFunction;

    NonAllocatableAggregateFunction* allocate(size_t /*count*/) {
        throw std::bad_alloc();
    }

    void deallocate(NonAllocatableAggregateFunction*, size_t /*count*/) {}
};

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
    StatelessHasPrefixFunction::callsCount = 0;
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
        decltype(rows) expected{2};
        REQUIRE(rows == expected);
    }

    {
        //  create function
        REQUIRE(StatelessHasPrefixFunction::callsCount == 0);
        REQUIRE(StatelessHasPrefixFunction::objectsCount == 0);
        storage.create_scalar_function<StatelessHasPrefixFunction>();
        REQUIRE(StatelessHasPrefixFunction::callsCount == 0);
        // function object created
        REQUIRE(StatelessHasPrefixFunction::objectsCount == 1);

        //  call after creation
        {
            auto rows = storage.select(func<StatelessHasPrefixFunction>("one", "o"));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        REQUIRE(StatelessHasPrefixFunction::callsCount == 1);
        REQUIRE(StatelessHasPrefixFunction::objectsCount == 1);
        {
            auto rows = storage.select(func<StatelessHasPrefixFunction>("two", "b"));
            decltype(rows) expected{false};
            REQUIRE(rows == expected);
        }
        REQUIRE(StatelessHasPrefixFunction::callsCount == 2);
        REQUIRE(StatelessHasPrefixFunction::objectsCount == 1);

        //  delete function
        storage.delete_scalar_function<StatelessHasPrefixFunction>();
        // function object destroyed
        REQUIRE(StatelessHasPrefixFunction::objectsCount == 0);
    }

    {
        //  create function
        REQUIRE(HasPrefixFunction::callsCount == 0);
        REQUIRE(HasPrefixFunction::objectsCount == 0);
        storage.create_scalar_function<HasPrefixFunction>();
        REQUIRE(HasPrefixFunction::callsCount == 0);
        REQUIRE(HasPrefixFunction::objectsCount == 0);

        //  call after creation
        {
            auto rows = storage.select(func<HasPrefixFunction>("one", "o"));
            decltype(rows) expected{true};
            REQUIRE(rows == expected);
        }
        REQUIRE(HasPrefixFunction::callsCount == 1);
        REQUIRE(HasPrefixFunction::objectsCount == 0);
        {
            auto rows = storage.select(func<HasPrefixFunction>("two", "b"));
            decltype(rows) expected{false};
            REQUIRE(rows == expected);
        }
        REQUIRE(HasPrefixFunction::callsCount == 2);
        REQUIRE(HasPrefixFunction::objectsCount == 0);

        //  delete function
        storage.delete_scalar_function<HasPrefixFunction>();
    }

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
        decltype(rows) expected{2};
        REQUIRE(rows == expected);
    }
    storage.delete_aggregate_function<MeanFunction>();

    storage.create_aggregate_function<MeanFunction>();
    // expect two different aggregate function objects to be created, which provide two different results;
    // This ensures that `proxy_get_aggregate_step_udf()` uses `sqlite3_aggregate_context()` correctly
    {
        MeanFunction::wasInstantiatedCount = 0;
        REQUIRE(MeanFunction::objectsCount == 0);
        REQUIRE(MeanFunction::wasInstantiatedCount == 0);
        auto rows = storage.select(columns(func<MeanFunction>(&User::id), func<MeanFunction>(c(&User::id) * 2)));
        REQUIRE(MeanFunction::objectsCount == 0);
        REQUIRE(MeanFunction::wasInstantiatedCount == 2);
        REQUIRE(int(std::get<0>(rows[0])) == 2);
        REQUIRE(int(std::get<1>(rows[0])) == 4);
    }
    storage.delete_aggregate_function<MeanFunction>();

    storage.create_aggregate_function<NonAllocatableAggregateFunction>();
    REQUIRE_THROWS_MATCHES(storage.select(func<NonAllocatableAggregateFunction>(&User::id)),
                           std::system_error,
                           ErrorCodeExceptionMatcher(sqlite_errc(SQLITE_NOMEM)));
    storage.delete_aggregate_function<NonAllocatableAggregateFunction>();

    storage.create_scalar_function<FirstFunction>();
    {
        auto rows = storage.select(func<FirstFunction>("Vanotek", "Tinashe", "Pitbull"));
        decltype(rows) expected{"VTP"};
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 1);
    }
    {
        auto rows = storage.select(func<FirstFunction>("Charli XCX", "Rita Ora"));
        decltype(rows) expected{"CR"};
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 2);
    }
    {
        auto rows = storage.select(func<FirstFunction>("Ted"));
        decltype(rows) expected{"T"};
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 3);
    }
    {
        auto rows = storage.select(func<FirstFunction>());
        decltype(rows) expected{""};
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 4);
    }
    storage.delete_scalar_function<FirstFunction>();

    storage.create_aggregate_function<MultiSum>();
    {
        REQUIRE(MultiSum::objectsCount == 0);
        auto rows = storage.select(func<MultiSum>(&User::id, 5));
        decltype(rows) expected{21};
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
    SECTION("escaped function identifier") {
        constexpr auto clamp_f = R"("clamp int")"_scalar.quote(std::clamp<int>);
        storage.create_scalar_function<clamp_f>();
        {
            auto rows = storage.select(clamp_f(0, 1, 1));
            decltype(rows) expected{1};
            REQUIRE(rows == expected);
        }
        storage.delete_scalar_function<clamp_f>();
    }
}
#endif
