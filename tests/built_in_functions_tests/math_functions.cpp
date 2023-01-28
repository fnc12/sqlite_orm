#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

#ifdef SQLITE_ENABLE_MATH_FUNCTIONS

constexpr double Epsilon = 0.0001;
static bool is_double_eq(double a, double b, double epsilon) {
    return ((a - b) < epsilon) && ((b - a) < epsilon);
}

TEST_CASE("math functions") {
    using namespace std::placeholders;
    auto storage = make_storage("");
    auto doubleComparator = std::bind(is_double_eq, _1, _2, Epsilon);
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    auto optionalComparator = [](const std::optional<double>& lhs, const std::optional<double>& rhs) {
        if(lhs.has_value() && rhs.has_value()) {
            return is_double_eq(*lhs, *rhs, Epsilon);
        } else if(!lhs.has_value() && !rhs.has_value()) {
            return true;
        } else {
            return false;
        }
    };
#endif
    SECTION("acos"){SECTION("simple"){auto rows = storage.select(sqlite_orm::acos(1));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::acos<std::optional<double>>(1));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("acosh"){SECTION("simple"){auto rows = storage.select(sqlite_orm::acosh(1));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::acosh<std::optional<double>>(1));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("asin"){SECTION("simple"){auto rows = storage.select(sqlite_orm::asin(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::asin<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("asinh"){SECTION("simple"){auto rows = storage.select(sqlite_orm::asinh(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::asinh<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("atan"){SECTION("simple"){auto rows = storage.select(sqlite_orm::atan(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::atan<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("atan2"){SECTION("simple"){auto rows = storage.select(sqlite_orm::atan2(0, 1));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::atan2<std::optional<double>>(0, 1));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("atanh"){SECTION("simple"){auto rows = storage.select(sqlite_orm::atanh(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::atanh<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("ceil"){SECTION("simple"){auto rows = storage.select(sqlite_orm::ceil(0.5));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::ceil<std::optional<double>>(0.5));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("ceiling"){SECTION("simple"){auto rows = storage.select(sqlite_orm::ceiling(0.5));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::ceiling<std::optional<double>>(0.5));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("cos"){SECTION("simple"){auto rows = storage.select(sqlite_orm::cos(0));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::cos<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("cosh"){SECTION("simple"){auto rows = storage.select(sqlite_orm::cosh(0));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::cosh<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("degrees"){SECTION("simple"){auto rows = storage.select(sqlite_orm::degrees(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::degrees<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("exp"){SECTION("simple"){auto rows = storage.select(sqlite_orm::exp(0));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::exp<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("floor"){SECTION("simple"){auto rows = storage.select(sqlite_orm::floor(1.5));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::floor<std::optional<double>>(1.5));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("ln"){SECTION("simple"){auto rows = storage.select(sqlite_orm::ln(1));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::ln<std::optional<double>>(1));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("log(x)"){SECTION("simple"){auto rows = storage.select(sqlite_orm::log(10));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), doubleComparator));
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::log<std::optional<double>>(10));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), optionalComparator));
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("log10"){SECTION("simple"){auto rows = storage.select(sqlite_orm::log10(10));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), doubleComparator));
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::log10<std::optional<double>>(10));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), optionalComparator));
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("log(b, x)"){SECTION("simple"){auto rows = storage.select(sqlite_orm::log(25, 625));
decltype(rows) expected;
expected.push_back(2);
REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), doubleComparator));
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::log<std::optional<double>>(25, 625));
    decltype(rows) expected;
    expected.push_back(2);
    REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), optionalComparator));
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("log2"){SECTION("simple"){auto rows = storage.select(sqlite_orm::log2(4));
decltype(rows) expected;
expected.push_back(2);
REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), doubleComparator));
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::log2<std::optional<double>>(4));
    decltype(rows) expected;
    expected.push_back(2);
    REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), optionalComparator));
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("mod"){SECTION("simple"){auto rows = storage.select(sqlite_orm::mod_f(6, 5));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::mod_f<std::optional<double>>(6, 5));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("pi"){SECTION("simple"){auto rows = storage.select(sqlite_orm::pi());
decltype(rows) expected;
expected.push_back(3.141592654);
REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), doubleComparator));
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::pi<std::optional<double>>());
    decltype(rows) expected;
    expected.push_back(3.141592654);
    REQUIRE(std::equal(rows.begin(), rows.end(), expected.begin(), optionalComparator));
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("pow"){SECTION("simple"){auto rows = storage.select(sqlite_orm::pow(2, 3));
decltype(rows) expected;
expected.push_back(8);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::pow<std::optional<double>>(2, 3));
    decltype(rows) expected;
    expected.push_back(8);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("power"){SECTION("simple"){auto rows = storage.select(sqlite_orm::power(2, 3));
decltype(rows) expected;
expected.push_back(8);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::power<std::optional<double>>(2, 3));
    decltype(rows) expected;
    expected.push_back(8);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("radians"){SECTION("simple"){auto rows = storage.select(sqlite_orm::radians(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::radians<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("sin"){SECTION("simple"){auto rows = storage.select(sqlite_orm::sin(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::sin<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("sinh"){SECTION("simple"){auto rows = storage.select(sqlite_orm::sinh(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::sinh<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("sqrt"){SECTION("simple"){auto rows = storage.select(sqlite_orm::sqrt(1));
decltype(rows) expected;
expected.push_back(1);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::sqrt<std::optional<double>>(1));
    decltype(rows) expected;
    expected.push_back(1);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("tan"){SECTION("simple"){auto rows = storage.select(sqlite_orm::tan(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::tan<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("tanh"){SECTION("simple"){auto rows = storage.select(sqlite_orm::tanh(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::tanh<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("tanh"){SECTION("simple"){auto rows = storage.select(sqlite_orm::tanh(0));
decltype(rows) expected;
expected.push_back(0);
REQUIRE(rows == expected);
}
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
SECTION("explicit type") {
    auto rows = storage.select(sqlite_orm::tanh<std::optional<double>>(0));
    decltype(rows) expected;
    expected.push_back(0);
    REQUIRE(rows == expected);
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
SECTION("trunc") {
    SECTION("simple") {
        auto rows = storage.select(sqlite_orm::trunc(1.5));
        decltype(rows) expected;
        expected.push_back(1);
        REQUIRE(rows == expected);
    }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    SECTION("explicit type") {
        auto rows = storage.select(sqlite_orm::trunc<std::optional<double>>(1.5));
        decltype(rows) expected;
        expected.push_back(1);
        REQUIRE(rows == expected);
    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
}

#endif  //  SQLITE_ENABLE_MATH_FUNCTIONS
