/**
 *  This example was taken from here http://souptonuts.sourceforge.net/readme_sqlite_tutorial.html
 */
#include <sqlite_orm/sqlite_orm.h>
#include <iostream>

using namespace sqlite_orm;
using std::cout;
using std::endl;

/**
 *  Scalar function must be defined as a dedicated class with at least two functions:
 *  1) `operator()` which takes any amount of arguments that can be obtained using row_extractor
 *  and returns the result of any type that can be bound using statement_binder.
 *  `operator()` can be both right const and non-right const.
 *  2) static `name` which return a name. Return type can be any and must have operator<<(std::ostream &) overload.
 *  You can use `std::string`, `std::string_view` or `const char *` for example.
 *  Note: If you want a function to accept variadic arguments make `operator()` accepting
 *  a single argument of type `(const arg_values&)`.
 */
struct SignFunction {

    double operator()(double arg) const {
        if(arg > 0) {
            return 1;
        } else if(arg < 0) {
            return -1;
        } else {
            return 0;
        }
    }

    static const char *name() {
        return "SIGN";
    }
};

/**
 *  Aggregate function must be defined as a dedicated class with at least three functions:
 *  1) `void step(...)` which can be called 0 or more times during one call inside single SQL query (once per row).
 *  It can accept any number of arguments that can be obtained using row_extractor.
 *  2) `fin() const` which is called once per call inside single SQL query. `fin` can have any result type
 *  that can be bound using statement_binder. Result of `fin` is a result of aggregate function.
 *  3) static `name` which return a name. Return type can be any and must have operator<<(std::ostream &) overload.
 *  You can use `std::string`, `std::string_view` or `const char *` for example.
 */
struct AcceleratedSumFunction {
    std::vector<int> list;

    void step(int value) {
        if(!this->list.empty()) {
            auto nextValue = list.back() + value;
            this->list.push_back(nextValue);
        } else {
            this->list.push_back(value);
        }
    }

    std::string fin() const {
        std::stringstream ss;
        ss << "(";
        for(size_t i = 0; i < this->list.size(); ++i) {
            ss << this->list[i];
            if(i < (this->list.size() - 1)) {
                ss << ", ";
            }
        }
        ss << ")";
        return ss.str();
    }

    /**
     *  `const std::string &` is also a valid type of name cause it has `operator<<(std::ostream &` overload
     */
    static const std::string &name() {
        static const std::string result = "ASUM";
        return result;
    }
};

/**
 *  This is also a scalar function just like `SignFunction` but this function
 *  can accept variadic amount of arguments. The only difference is arguments list
 *  of `operator()`: it is `const arg_values &`. `arg_values` has STL container API.
 */
struct ArithmeticMeanFunction {

    double operator()(const arg_values &args) const {
        double result = 0;
        for(auto arg_value: args) {
            if(arg_value.is_float()) {
                result += arg_value.get<double>();
            } else if(arg_value.is_integer()) {
                result += arg_value.get<int>();
            }
        }
        if(!args.empty()) {
            result /= double(args.size());
        }
        return result;
    }

    static const std::string &name() {
        static const std::string result = "ARITHMETIC_MEAN";
        return result;
    }
};

int main() {

    struct Table {
        int a = 0;
        int b = 0;
        int c = 0;
    };

    auto storage = make_storage(
        {},
        make_table("t", make_column("a", &Table::a), make_column("b", &Table::b), make_column("c", &Table::c)));
    storage.sync_schema();

    /**
     *  This function can be called at any time doesn't matter whether connection is open or not.
     *  To delete created scalar function use `storage.delete_scalar_function<T>()` function call.
     */
    storage.create_scalar_function<SignFunction>();

    //  SELECT SIGN(3)
    auto signRows = storage.select(func<SignFunction>(3));
    cout << "SELECT SIGN(3) = " << signRows.at(0) << endl;

    storage.insert(Table{1, -1, 2});
    storage.insert(Table{2, -2, 4});
    storage.insert(Table{3, -3, 8});
    storage.insert(Table{4, -4, 16});

    storage.create_aggregate_function<AcceleratedSumFunction>();

    //  SELECT ASUM(a), ASUM(b), ASUM(c)
    //  FROM t
    auto aSumRows = storage.select(columns(func<AcceleratedSumFunction>(&Table::a),
                                           func<AcceleratedSumFunction>(&Table::b),
                                           func<AcceleratedSumFunction>(&Table::c)));
    cout << "SELECT ASUM(a), ASUM(b), ASUM(c) FROM t:" << endl;
    for(auto &row: aSumRows) {
        cout << '\t' << get<0>(row) << endl;
        cout << '\t' << get<1>(row) << endl;
        cout << '\t' << get<2>(row) << endl;
    }

    storage.create_scalar_function<ArithmeticMeanFunction>();

    //  SELECT ARITHMETIC_MEAN(5, 6, 7)
    auto arithmeticMeanRows1 = storage.select(func<ArithmeticMeanFunction>(5, 6, 7));
    cout << "SELECT ARITHMETIC_MEAN(5, 6, 7) = " << arithmeticMeanRows1.front() << endl;

    //  SELECT ARITHMETIC_MEAN(-2, 1)
    auto arithmeticMeanRows2 = storage.select(func<ArithmeticMeanFunction>(-2, 1));
    cout << "SELECT ARITHMETIC_MEAN(-2, 1) = " << arithmeticMeanRows2.front() << endl;

    //  SELECT ARITHMETIC_MEAN(-5.5, 4, 13.2, 256.4)
    auto arithmeticMeanRows3 = storage.select(func<ArithmeticMeanFunction>(-5.5, 4, 13.2, 256.4));
    cout << "SELECT ARITHMETIC_MEAN(-5.5, 4, 13.2, 256.4) = " << arithmeticMeanRows3.front() << endl;

    return 0;
}
