#pragma once
#include <system_error>
#include <sstream>
#include <optional>  //  std::optional
#include <vector>  //  std::vector
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

class ErrorCodeExceptionMatcher : public Catch::Matchers::MatcherGenericBase {
  public:
    ErrorCodeExceptionMatcher(std::error_code errorCode) : errorCode(std::move(errorCode)) {}

    bool match(const std::system_error& systemError) const {
        return systemError.code() == this->errorCode;
    }

  protected:
    std::string describe() const override {
        std::stringstream ss;
        ss << this->errorCode;
        return ss.str();
    }

  private:
    std::error_code errorCode;
};

/**
 *  Matches a range of nullable elements (e.g. `std::unique_ptr`, `std::optional`) against the expected values
 *  by comparing what they hold; `std::nullopt` expects an empty element.
 */
template<class T>
auto PointeesEqual(std::vector<std::optional<T>> expected) {
    return Catch::Matchers::RangeEquals(std::move(expected), [](const auto& actual, const auto& value) {
        return bool(actual) == bool(value) && (!actual || *actual == *value);
    });
}
