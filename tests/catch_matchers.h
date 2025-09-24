#pragma once
#include <system_error>
#include <sstream>
#include <catch2/matchers/catch_matchers_templated.hpp>

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
