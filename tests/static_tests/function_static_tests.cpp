#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;
using internal::is_aggregate_function_v;
using internal::is_scalar_function_v;

TEST_CASE("function static") {
    SECTION("scalar") {
        SECTION("variadic") {
            struct FirstFunction {
                std::string operator()(const arg_values &args) const {
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
        }
        SECTION("non variadic") {
            SECTION("double(double) const") {
                struct Function {
                    double operator()(double arg) const {
                        return std::sqrt(arg);
                    }
                };

                STATIC_REQUIRE(is_scalar_function_v<Function>);
                STATIC_REQUIRE(!is_aggregate_function_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = double (Function::*)(double) const;
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
                using ExpectedArgumentsTuple = std::tuple<double>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, double>::value);
                STATIC_REQUIRE(
                    std::is_same<internal::callable_arguments<Function>::args_tuple, std::tuple<double>>::value);
            }
            SECTION("double(double)") {
                struct Function {
                    double operator()(double arg) {
                        return std::sqrt(arg);
                    }
                };

                STATIC_REQUIRE(is_scalar_function_v<Function>);
                STATIC_REQUIRE(!is_aggregate_function_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = double (Function::*)(double);
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
                using ExpectedArgumentsTuple = std::tuple<double>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, double>::value);
                STATIC_REQUIRE(
                    std::is_same<internal::callable_arguments<Function>::args_tuple, std::tuple<double>>::value);
            }
            SECTION("int(std::string) const") {
                struct Function {
                    int operator()(std::string arg) const {
                        return int(arg.length());
                    }
                };

                STATIC_REQUIRE(is_scalar_function_v<Function>);
                STATIC_REQUIRE(!is_aggregate_function_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = int (Function::*)(std::string) const;
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
                using ExpectedArgumentsTuple = std::tuple<std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, int>::value);
                STATIC_REQUIRE(
                    std::is_same<internal::callable_arguments<Function>::args_tuple, std::tuple<std::string>>::value);
            }
            SECTION("int(std::string)") {
                struct Function {
                    int operator()(std::string arg) {
                        return int(arg.length());
                    }
                };

                STATIC_REQUIRE(is_scalar_function_v<Function>);
                STATIC_REQUIRE(!is_aggregate_function_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = int (Function::*)(std::string);
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
                using ExpectedArgumentsTuple = std::tuple<std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, int>::value);
                STATIC_REQUIRE(
                    std::is_same<internal::callable_arguments<Function>::args_tuple, std::tuple<std::string>>::value);
            }
            SECTION("std::string(const std::string &, const std::string &) const") {
                struct Function {
                    std::string operator()(const std::string &arg1, const std::string &arg2) const {
                        return arg1 + arg2;
                    }
                };

                STATIC_REQUIRE(is_scalar_function_v<Function>);
                STATIC_REQUIRE(!is_aggregate_function_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = std::string (Function::*)(const std::string &, const std::string &) const;
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
                using ExpectedArgumentsTuple = std::tuple<std::string, std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, std::string>::value);
                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::args_tuple,
                                            std::tuple<std::string, std::string>>::value);
            }
            SECTION("std::string(const std::string &, const std::string &)") {
                struct Function {
                    std::string operator()(const std::string &arg1, const std::string &arg2) {
                        return arg1 + arg2;
                    }
                };

                STATIC_REQUIRE(is_scalar_function_v<Function>);
                STATIC_REQUIRE(!is_aggregate_function_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = std::string (Function::*)(const std::string &, const std::string &);
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::member_function_arguments<RunMemberFunctionPointer>::tuple_type;
                using ExpectedArgumentsTuple = std::tuple<std::string, std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, std::string>::value);
                STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::args_tuple,
                                            std::tuple<std::string, std::string>>::value);
            }
        }
    }
    SECTION("aggregate") {
        SECTION("void(int) & int() const") {
            struct Function {
                int total = 0;
                int count = 0;

                void step(int value) {
                    ++count;
                    total += value;
                }

                int fin() const {
                    return total;
                }
            };

            STATIC_REQUIRE(is_aggregate_function_v<Function>);
            STATIC_REQUIRE(!is_scalar_function_v<Function>);

            using StepMemberFunctionPointer = internal::aggregate_step_function_t<Function>;
            using ExpectedStepType = void (Function::*)(int);
            STATIC_REQUIRE(std::is_same<StepMemberFunctionPointer, ExpectedStepType>::value);

            using FinMemberFunctionPointer = internal::aggregate_fin_function_t<Function>;
            using ExpectedFinType = int (Function::*)() const;
            STATIC_REQUIRE(std::is_same<FinMemberFunctionPointer, ExpectedFinType>::value);

            STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, int>::value);
            STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::args_tuple, std::tuple<int>>::value);
        }
        SECTION("void(std::string) const & std::string()") {
            struct Function {
                mutable std::string result;

                void step(std::string value) const {
                    result += value[0];
                }

                std::string fin() {
                    return move(result);
                }
            };

            STATIC_REQUIRE(is_aggregate_function_v<Function>);
            STATIC_REQUIRE(!is_scalar_function_v<Function>);

            using StepMemberFunctionPointer = internal::aggregate_step_function_t<Function>;
            using ExpectedStepType = void (Function::*)(std::string) const;
            STATIC_REQUIRE(std::is_same<StepMemberFunctionPointer, ExpectedStepType>::value);

            using FinMemberFunctionPointer = internal::aggregate_fin_function_t<Function>;
            using ExpectedFinType = std::string (Function::*)();
            STATIC_REQUIRE(std::is_same<FinMemberFunctionPointer, ExpectedFinType>::value);

            STATIC_REQUIRE(std::is_same<internal::callable_arguments<Function>::return_type, std::string>::value);
            STATIC_REQUIRE(
                std::is_same<internal::callable_arguments<Function>::args_tuple, std::tuple<std::string>>::value);
        }
    }
}
