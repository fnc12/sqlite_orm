#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("function static") {
    SECTION("double(double) const") {
        struct Function {
            double operator()(double arg) const {
                return std::sqrt(arg);
            }
        };

        using RunMemberFunctionPointer = internal::run_member_pointer<Function>::type;
        using ExpectedType = double (Function::*)(double) const;
        static_assert(std::is_same<RunMemberFunctionPointer, ExpectedType>::value, "");

        using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
        using ExpectedArgumentsTuple = std::tuple<double>;
        static_assert(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value, "");
    }
    SECTION("int(std::string) const") {
        struct Function {
            int operator()(std::string arg) const {
                return int(arg.length());
            }
        };

        using RunMemberFunctionPointer = internal::run_member_pointer<Function>::type;
        using ExpectedType = int (Function::*)(std::string) const;
        static_assert(std::is_same<RunMemberFunctionPointer, ExpectedType>::value, "");

        using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
        using ExpectedArgumentsTuple = std::tuple<std::string>;
        static_assert(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value, "");
    }
    SECTION("std::string(const std::string &, const std::string &) const") {
        struct Function {
            std::string operator()(const std::string &arg1, const std::string &arg2) const {
                return arg1 + arg2;
            }
        };

        using RunMemberFunctionPointer = internal::run_member_pointer<Function>::type;
        using ExpectedType = std::string (Function::*)(const std::string &, const std::string &) const;
        static_assert(std::is_same<RunMemberFunctionPointer, ExpectedType>::value, "");

        using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
        using ExpectedArgumentsTuple = std::tuple<std::string, std::string>;
        static_assert(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value, "");
    }
}
