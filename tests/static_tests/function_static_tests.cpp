#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;
using internal::callable_arguments;
using internal::function;
using internal::function_call;
using internal::is_aggregate_udf_v;
using internal::is_scalar_udf_v;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
using internal::quoted_scalar_function;
#endif

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
template<class S, auto f>
concept storage_scalar_callable = requires(S& storage) {
    { storage.create_scalar_function<f>() };
    { storage.delete_scalar_function<f>() };
};

template<class S, auto f>
concept storage_aggregate_callable = requires(S& storage) {
    { storage.create_aggregate_function<f>() };
    { storage.delete_aggregate_function<f>() };
};
#endif

TEST_CASE("function static") {
    SECTION("scalar") {
        SECTION("variadic") {
            struct FirstFunction {
                std::string operator()(const arg_values& args) const {
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
        }
        SECTION("non variadic") {
            SECTION("double(double) const") {
                struct Function {
                    double operator()(double arg) const {
                        return std::sqrt(arg);
                    }
                };

                STATIC_REQUIRE(is_scalar_udf_v<Function>);
                STATIC_REQUIRE_FALSE(is_aggregate_udf_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = double (Function::*)(double) const;
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::function_arguments<RunMemberFunctionPointer, std::tuple, std::decay_t>;
                using ExpectedArgumentsTuple = std::tuple<double>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, double>::value);
                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple, std::tuple<double>>::value);
            }
            SECTION("double(double)") {
                struct Function {
                    double operator()(double arg) {
                        return std::sqrt(arg);
                    }
                };

                STATIC_REQUIRE(is_scalar_udf_v<Function>);
                STATIC_REQUIRE_FALSE(is_aggregate_udf_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = double (Function::*)(double);
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::function_arguments<RunMemberFunctionPointer, std::tuple, std::decay_t>;
                using ExpectedArgumentsTuple = std::tuple<double>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, double>::value);
                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple, std::tuple<double>>::value);
            }
            SECTION("int(std::string) const") {
                struct Function {
                    int operator()(std::string arg) const {
                        return int(arg.length());
                    }
                };

                STATIC_REQUIRE(is_scalar_udf_v<Function>);
                STATIC_REQUIRE_FALSE(is_aggregate_udf_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = int (Function::*)(std::string) const;
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::function_arguments<RunMemberFunctionPointer, std::tuple, std::decay_t>;
                using ExpectedArgumentsTuple = std::tuple<std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, int>::value);
                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple, std::tuple<std::string>>::value);
            }
            SECTION("int(std::string)") {
                struct Function {
                    int operator()(std::string arg) {
                        return int(arg.length());
                    }
                };

                STATIC_REQUIRE(is_scalar_udf_v<Function>);
                STATIC_REQUIRE_FALSE(is_aggregate_udf_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = int (Function::*)(std::string);
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::function_arguments<RunMemberFunctionPointer, std::tuple, std::decay_t>;
                using ExpectedArgumentsTuple = std::tuple<std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, int>::value);
                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple, std::tuple<std::string>>::value);
            }
            SECTION("std::string(const std::string &, const std::string &) const") {
                struct Function {
                    std::string operator()(const std::string& arg1, const std::string& arg2) const {
                        return arg1 + arg2;
                    }
                };

                STATIC_REQUIRE(is_scalar_udf_v<Function>);
                STATIC_REQUIRE_FALSE(is_aggregate_udf_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = std::string (Function::*)(const std::string&, const std::string&) const;
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::function_arguments<RunMemberFunctionPointer, std::tuple, std::decay_t>;
                using ExpectedArgumentsTuple = std::tuple<std::string, std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, std::string>::value);
                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple,
                                            std::tuple<std::string, std::string>>::value);
            }
            SECTION("std::string(const std::string &, const std::string &)") {
                struct Function {
                    std::string operator()(const std::string& arg1, const std::string& arg2) {
                        return arg1 + arg2;
                    }
                };

                STATIC_REQUIRE(is_scalar_udf_v<Function>);
                STATIC_REQUIRE_FALSE(is_aggregate_udf_v<Function>);

                using RunMemberFunctionPointer = internal::scalar_call_function_t<Function>;
                using ExpectedType = std::string (Function::*)(const std::string&, const std::string&);
                STATIC_REQUIRE(std::is_same<RunMemberFunctionPointer, ExpectedType>::value);

                using ArgumentsTuple = internal::function_arguments<RunMemberFunctionPointer, std::tuple, std::decay_t>;
                using ExpectedArgumentsTuple = std::tuple<std::string, std::string>;
                STATIC_REQUIRE(std::is_same<ArgumentsTuple, ExpectedArgumentsTuple>::value);

                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, std::string>::value);
                STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple,
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

            STATIC_REQUIRE(is_aggregate_udf_v<Function>);
            STATIC_REQUIRE_FALSE(is_scalar_udf_v<Function>);

            using StepMemberFunctionPointer = internal::aggregate_step_function_t<Function>;
            using ExpectedStepType = void (Function::*)(int);
            STATIC_REQUIRE(std::is_same<StepMemberFunctionPointer, ExpectedStepType>::value);

            using FinMemberFunctionPointer = internal::aggregate_fin_function_t<Function>;
            using ExpectedFinType = int (Function::*)() const;
            STATIC_REQUIRE(std::is_same<FinMemberFunctionPointer, ExpectedFinType>::value);

            STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, int>::value);
            STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple, std::tuple<int>>::value);
        }
        SECTION("void(std::string) const & std::string()") {
            struct Function {
                mutable std::string result;

                void step(std::string value) const {
                    result += value[0];
                }

                std::string fin() {
                    return std::move(result);
                }
            };

            STATIC_REQUIRE(is_aggregate_udf_v<Function>);
            STATIC_REQUIRE_FALSE(is_scalar_udf_v<Function>);

            using StepMemberFunctionPointer = internal::aggregate_step_function_t<Function>;
            using ExpectedStepType = void (Function::*)(std::string) const;
            STATIC_REQUIRE(std::is_same<StepMemberFunctionPointer, ExpectedStepType>::value);

            using FinMemberFunctionPointer = internal::aggregate_fin_function_t<Function>;
            using ExpectedFinType = std::string (Function::*)();
            STATIC_REQUIRE(std::is_same<FinMemberFunctionPointer, ExpectedFinType>::value);

            STATIC_REQUIRE(std::is_same<callable_arguments<Function>::return_type, std::string>::value);
            STATIC_REQUIRE(std::is_same<callable_arguments<Function>::args_tuple, std::tuple<std::string>>::value);
        }
    }
    SECTION("function call expressions") {
        struct SFunction {
            static const char* name() {
                return "";
            }
            int operator()(int) const;
        };
        struct AFunction {
            static const char* name() {
                return "";
            }
            void step(int);
            int fin() const;
        };

        constexpr auto scalar = func<SFunction>;
        constexpr auto aggregate = func<AFunction>;

        STATIC_REQUIRE(std::is_same<decltype(scalar), const function<SFunction>>::value);
        STATIC_REQUIRE(std::is_same<decltype(scalar(42)), function_call<SFunction, int>>::value);
        STATIC_REQUIRE(std::is_same<decltype(aggregate), const function<AFunction>>::value);
        STATIC_REQUIRE(std::is_same<decltype(aggregate(42)), function_call<AFunction, int>>::value);

        STATIC_REQUIRE(std::is_same_v<function<SFunction>::callable_type, SFunction>);
        STATIC_REQUIRE(std::is_same_v<function<SFunction>::udf_type, SFunction>);

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        STATIC_REQUIRE(orm_scalar_function<decltype(scalar)>);
        STATIC_REQUIRE_FALSE(orm_aggregate_function<decltype(scalar)>);
        STATIC_REQUIRE(orm_aggregate_function<decltype(aggregate)>);
        STATIC_REQUIRE_FALSE(orm_scalar_function<decltype(aggregate)>);

        using storage_type = decltype(make_storage(""));
        STATIC_REQUIRE(storage_scalar_callable<storage_type, scalar>);
        STATIC_REQUIRE_FALSE(storage_aggregate_callable<storage_type, scalar>);
        STATIC_REQUIRE(storage_aggregate_callable<storage_type, aggregate>);
        STATIC_REQUIRE_FALSE(storage_scalar_callable<storage_type, aggregate>);
#endif
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("quoted") {
        constexpr auto quotedScalar = "f"_scalar.quote(std::clamp<int>);
        using quoted_type = decltype("f"_scalar.quote(std::clamp<int>));

        STATIC_REQUIRE(quotedScalar.nme[0] == 'f' && quotedScalar.nme[1] == '\0');
        STATIC_REQUIRE(std::is_same_v<decltype(quotedScalar),
                                      const quoted_scalar_function<decltype(std::clamp<int>)*,
                                                                   const int&(const int&, const int&, const int&),
                                                                   2>>);

        STATIC_REQUIRE(std::is_same_v<quoted_type::callable_type, decltype(std::clamp<int>)*>);
        STATIC_REQUIRE(std::is_same_v<quoted_type::udf_type, const int&(const int&, const int&, const int&)>);

        STATIC_REQUIRE(std::is_same_v<callable_arguments<quoted_type::udf_type>::return_type, int>);
        STATIC_REQUIRE(
            std::is_same_v<callable_arguments<quoted_type::udf_type>::args_tuple, std::tuple<int, int, int>>);

        STATIC_REQUIRE(orm_quoted_scalar_function<decltype(quotedScalar)>);
        STATIC_REQUIRE_FALSE(orm_scalar_function<decltype(quotedScalar)>);

        STATIC_REQUIRE(std::is_same_v<decltype(quotedScalar(0, 1, 1)),
                                      function_call<const int&(const int&, const int&, const int&), int, int, int>>);

        using storage_type = decltype(make_storage(""));
        STATIC_REQUIRE(storage_scalar_callable<storage_type, quotedScalar>);
    }
#endif
}
