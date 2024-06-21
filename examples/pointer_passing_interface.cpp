/**
 *  Example of how to use the pointer-passing interface, creating an ad-hoc sqlite extension.
 *
 *  It demonstrates how to modularize turning a seralized representation of an object
 *  into C/C++ native object instance, plus invoking operations on that object.
 *  This approach is much better in regard to encapsulation in contrary to defining a set of
 *  functions which always only accept the serialized representation of an object.
 *
 *  This example assumes an application that deals with a fixed set of c++ system error categories and codes,
 *  which are also stored in a result table:
 *    - stores results of some type, along with a error category enumeration and an error code.
 *    - registers a sqlite scalar function getting the pointer to the error category.
 *    - registers another set of sqlite scalar functions that accept a pointer to an error category.
 *    - registers a sqlite scalar function creating a std::error_code.
 *    - registers a sqlite scalar function comparing two std::error_code.
 *
 *  Note: pointers are only accessible within application code, and therefore unleakable.
 */
#include <sqlite_orm/sqlite_orm.h>
#if SQLITE_VERSION_NUMBER >= 3020000
#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#define ENABLE_THIS_EXAMPLE
#endif
#endif

#ifdef ENABLE_THIS_EXAMPLE
#include <type_traits>
#include <tuple>
#include <system_error>
#include <future>
#include <string>
#include <algorithm>
#include <iostream>

using namespace sqlite_orm;

using std::cout;
using std::default_delete;
using std::endl;
using std::error_category;
using std::error_code;
using std::make_unique;
using std::min;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
// c++ integral constant for our pointer type domain
inline constexpr orm_pointer_type auto ecode_pointer_tag = "ecode"_pointer_type;
#else
// name for our pointer type domain
inline constexpr const char ecode_pvt_name[] = "ecode";
// c++ integral constant for our pointer type domain
using ecode_pointer_type = std::integral_constant<const char*, ecode_pvt_name>;
#endif

// a fixed set of error categories the application is dealing with
enum class app_error_category : unsigned int {
    generic,
    system,
    io,
    future,
    sqlite_orm,
};

// map app_error_category enum to std::error_category
// sorted by app_error_category
using ecat_map_t = std::tuple<const app_error_category, const std::error_category&>;
extern const std::array<ecat_map_t, 5> ecat_map;

const std::array<ecat_map_t, 5> ecat_map = {{
    {app_error_category::generic, std::generic_category()},
    {app_error_category::system, std::system_category()},
    {app_error_category::io, std::iostream_category()},
    {app_error_category::future, std::future_category()},
    {app_error_category::sqlite_orm, sqlite_orm::get_sqlite_error_category()},
}};

template<char A, char... L>
struct str_alias : alias_tag {
    static constexpr char str[] = {A, L..., '\0'};
    static constexpr const char* get() {
        return str;
    }
};
#endif

int main() {
#ifdef ENABLE_THIS_EXAMPLE
    // table structure for results of some type
    struct Result {
        int id = 0;
        int errorValue = 0;
        unsigned int errorCategory = 0;
    };
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    using ecat_arg = pointer_arg_t<const std::error_category, ecode_pointer_tag>;
    using ecode_arg = pointer_arg_t<const std::error_code, ecode_pointer_tag>;
#else
    using ecat_arg = pointer_arg<const std::error_category, ecode_pointer_type>;
    using ecode_arg = pointer_arg<const std::error_code, ecode_pointer_type>;
#endif

    // function returning a pointer to a std::error_category,
    // which is only visible to functions accepting pointer values of type "ecat"
    struct get_error_category_fn {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        using ecat_binding = static_pointer_binding_t<const std::error_category, ecode_pointer_tag>;
#else
        using ecat_binding = static_pointer_binding<const std::error_category, ecode_pointer_type>;
#endif

        ecat_binding operator()(unsigned int errorCategory) const {
            size_t idx = min<size_t>(errorCategory, ecat_map.size());
            const error_category* ecat = idx != ecat_map.size() ? &get<const error_category&>(ecat_map[idx]) : nullptr;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            return bind_pointer_statically<ecode_pointer_tag>(ecat);
#else
            return bind_pointer_statically<ecode_pointer_type>(ecat);
#endif
        }

        static constexpr const char* name() {
            return "get_error_category";
        }
    };

    // function accepting a pointer to a std::error_category,
    // returns the category's name
    struct error_category_name_fn {
        std::string operator()(ecat_arg pv) const {
            if(const error_category* ec = pv) {
                return ec->name();
            }
            return {};
        }

        static constexpr const char* name() {
            return "error_category_name";
        }
    };

    // function accepting a pointer to a std::error_category and an error code,
    // returns the error message
    struct error_category_message_fn {
        std::string operator()(ecat_arg pv, int errorValue) const {
            if(const error_category* ec = pv) {
                return ec->message(errorValue);
            }
            return {};
        }

        static constexpr const char* name() {
            return "error_category_message";
        }
    };

    // function returning an error_code object from an error value
    struct make_error_code_fn {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        using ecode_binding =
            pointer_binding_t<std::error_code, ecode_pointer_tag, std::default_delete<std::error_code>>;
#else
        using ecode_binding =
            pointer_binding<std::error_code, ecode_pointer_type, std::default_delete<std::error_code>>;
#endif

        ecode_binding operator()(int errorValue, unsigned int errorCategory) const {
            size_t idx = min<size_t>(errorCategory, ecat_map.size());
            error_code* ec = idx != ecat_map.size()
                                 ? new error_code{errorValue, get<const error_category&>(ecat_map[idx])}
                                 : nullptr;
            return bind_pointer<ecode_binding>(ec, default_delete<error_code>{});
        }

        static constexpr const char* name() {
            return "make_error_code";
        }
    };

    // function comparing two error_code objects
    struct equal_error_code_fn {
        bool operator()(ecode_arg pv1, ecode_arg pv2) const {
            const error_code *ec1 = pv1, *ec2 = pv2;
            if(ec1 && ec2) {
                return *ec1 == *ec2;
            }
            return false;
        }

        static constexpr const char* name() {
            return "equal_error_code";
        }
    };

    auto storage = make_storage({},
                                make_table("result",
                                           make_column("id", &Result::id, primary_key()),
                                           make_column("error_value", &Result::errorValue),
                                           make_column("error_category", &Result::errorCategory)));
    storage.sync_schema();

    storage.transaction([&storage] {
        storage.replace(Result{1, 2, 0});
        storage.replace(Result{2, 0, 1});
        storage.replace(Result{3, 1, 2});
        storage.replace(Result{4, 1, 3});
        storage.replace(Result{5, 0, 4});
        // error category out of bounds, get_error_category() will return null
        storage.replace(Result{6, 0, 5});
        return true;
    });

    storage.create_scalar_function<get_error_category_fn>();
    storage.create_scalar_function<error_category_name_fn>();
    storage.create_scalar_function<error_category_message_fn>();
    storage.create_scalar_function<make_error_code_fn>();
    storage.create_scalar_function<equal_error_code_fn>();

    //  list all results including error category name and error message
    {

        //  SELECT id, not error_value as ok, error_category, error_value,
        //      equal_error_code(make_error_code(error_value, error_category), ?),
        //      error_category_name(get_error_category(error_category)),
        //      error_category_message(get_error_category(error_category), error_value)
        //      FROM result
        //      ORDER BY not error_value, error_category, error_value, id;
        auto rows =
            storage.select(columns(&Result::id,
                                   as<str_alias<'o', 'k'>>(c(&Result::errorValue) == 0),
                                   &Result::errorValue,
                                   &Result::errorCategory,
                                   as<str_alias<'e', 'q'>>(func<equal_error_code_fn>(
                                       func<make_error_code_fn>(&Result::errorValue, &Result::errorCategory),
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
                                       bind_pointer<ecode_pointer_tag>(make_unique<error_code>()))),
#else
                                       bind_pointer<ecode_pointer_type>(make_unique<error_code>()))),
#endif
                                   func<error_category_name_fn>(func<get_error_category_fn>(&Result::errorCategory)),
                                   func<error_category_message_fn>(func<get_error_category_fn>(&Result::errorCategory),
                                                                   &Result::errorValue)),
                           multi_order_by(order_by(c(&Result::errorValue) == 0),
                                          order_by(&Result::errorCategory),
                                          order_by(&Result::errorValue),
                                          order_by(&Result::id)));
        for(auto& row: rows) {
            cout << std::get<0>(row) << ' ' << std::get<1>(row) << ' ' << std::get<2>(row) << ' ' << std::get<3>(row)
                 << ' ' << std::get<4>(row) << " \"" << std::get<5>(row) << "\""
                 << " \"" << std::get<6>(row) << "\"" << endl;
        }
        cout << endl;
    }

#endif
    return 0;
}
