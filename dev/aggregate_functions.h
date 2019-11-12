#pragma once

namespace sqlite_orm {

    namespace aggregate_functions {

        struct avg_string {
            operator std::string() const {
                return "AVG";
            }
        };

        /**
         *  Result of avg(...) call.
         *  T is an argument type
         */
        template<class T>
        struct avg_t : avg_string {
            using arg_type = T;

            arg_type arg;

            avg_t(arg_type arg_) : arg(std::move(arg_)) {}
        };

        struct count_string {
            operator std::string() const {
                return "COUNT";
            }
        };

        /**
         *  Result of count(...) call.
         *  T is an argument type
         */
        template<class T>
        struct count_t : count_string {
            using arg_type = T;

            arg_type arg;

            count_t(arg_type arg_) : arg(std::move(arg_)) {}
        };

        /**
         *  T is use to specify type explicitly for queries like
         *  SELECT COUNT(*) FROM table_name;
         *  T can be omitted with void.
         */
        template<class T>
        struct count_asterisk_t : count_string {
            using type = T;
        };

        /**
         *  The same thing as count<T>() but without T arg.
         *  Is used in cases like this:
         *    SELECT cust_code, cust_name, cust_city, grade
         *    FROM customer
         *    WHERE grade=2 AND EXISTS
         *        (SELECT COUNT(*)
         *        FROM customer
         *        WHERE grade=2
         *        GROUP BY grade
         *        HAVING COUNT(*)>2);
         *  `c++`
         *  auto rows =
         *      storage.select(columns(&Customer::code, &Customer::name, &Customer::city, &Customer::grade),
         *          where(is_equal(&Customer::grade, 2)
         *              and exists(select(count<Customer>(),
         *                  where(is_equal(&Customer::grade, 2)),
         *          group_by(&Customer::grade),
         *          having(greater_than(count(), 2))))));
         */
        struct count_asterisk_without_type : count_string {};

        struct sum_string {
            operator std::string() const {
                return "SUM";
            }
        };

        /**
         *  Result of sum(...) call.
         *  T is an argument type
         */
        template<class T>
        struct sum_t : sum_string {
            using arg_type = T;

            arg_type arg;

            sum_t(arg_type arg_) : arg(std::move(arg_)) {}
        };

        struct total_string {
            operator std::string() const {
                return "TOTAL";
            }
        };

        /**
         *  Result of total(...) call.
         *  T is an argument type
         */
        template<class T>
        struct total_t : total_string {
            using arg_type = T;

            arg_type arg;

            total_t(arg_type arg_) : arg(std::move(arg_)) {}
        };

        struct max_string {
            operator std::string() const {
                return "MAX";
            }
        };

        /**
         *  Result of max(...) call.
         *  T is an argument type
         */
        template<class T>
        struct max_t : max_string {
            using arg_type = T;

            arg_type arg;

            max_t(arg_type arg_) : arg(std::move(arg_)) {}
        };

        struct min_string {
            operator std::string() const {
                return "MIN";
            }
        };

        /**
         *  Result of min(...) call.
         *  T is an argument type
         */
        template<class T>
        struct min_t : min_string {
            using arg_type = T;

            arg_type arg;

            min_t(arg_type arg_) : arg(std::move(arg_)) {}
        };

        struct group_concat_string {
            operator std::string() const {
                return "GROUP_CONCAT";
            }
        };

        /**
         *  Result of group_concat(X) call.
         *  T is an argument type
         */
        template<class T>
        struct group_concat_single_t : group_concat_string {
            using arg_type = T;

            arg_type arg;

            group_concat_single_t(arg_type arg_) : arg(std::move(arg_)) {}
        };

        struct group_concat_double_base : group_concat_string {
            std::string y;

            group_concat_double_base(std::string y_) : y(move(y_)) {}
        };

        /**
         *  Result of group_concat(X, Y) call.
         *  T is an argument type
         */
        template<class T>
        struct group_concat_double_t : group_concat_double_base {
            using arg_type = T;

            arg_type arg;

            group_concat_double_t(arg_type arg_, std::string y) :
                group_concat_double_base(move(y)), arg(std::move(arg_)) {}
        };
    }

    /**
     *  AVG(X) aggregate function.
     */
    template<class T>
    aggregate_functions::avg_t<T> avg(T t) {
        return {std::move(t)};
    }

    /**
     *  COUNT(X) aggregate function.
     */
    template<class T>
    aggregate_functions::count_t<T> count(T t) {
        return {std::move(t)};
    }

    /**
     *  COUNT(*) without FROM function.
     */
    inline aggregate_functions::count_asterisk_without_type count() {
        return {};
    }

    /**
     *  COUNT(*) with FROM function. Specified type T will be serializeed as
     *  a from argument.
     */
    template<class T>
    aggregate_functions::count_asterisk_t<T> count() {
        return {};
    }

    /**
     *  SUM(X) aggregate function.
     */
    template<class T>
    aggregate_functions::sum_t<T> sum(T t) {
        return {std::move(t)};
    }

    /**
     *  MAX(X) aggregate function.
     */
    template<class T>
    aggregate_functions::max_t<T> max(T t) {
        return {std::move(t)};
    }

    /**
     *  MIN(X) aggregate function.
     */
    template<class T>
    aggregate_functions::min_t<T> min(T t) {
        return {std::move(t)};
    }

    /**
     *  TOTAL(X) aggregate function.
     */
    template<class T>
    aggregate_functions::total_t<T> total(T t) {
        return {std::move(t)};
    }

    /**
     *  GROUP_CONCAT(X) aggregate function.
     */
    template<class T>
    aggregate_functions::group_concat_single_t<T> group_concat(T t) {
        return {std::move(t)};
    }

    /**
     *  GROUP_CONCAT(X, Y) aggregate function.
     */
    template<class T, class Y>
    aggregate_functions::group_concat_double_t<T> group_concat(T t, Y y) {
        return {std::move(t), std::move(y)};
    }
}
