#pragma once

/** @file sqlite_orm's own error codes, `orm_error_code`, and their `std::error_category`.
 *        SQLite's result codes are in `sqlite3/sqlite3_errors.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <system_error>  //  std::error_code, std::error_category, std::is_error_code_enum
#include <string>  //  std::string
#include <type_traits>  //  std::true_type
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {
    enum class orm_error_code {
        not_found = 1,
        type_is_not_mapped_to_storage,
        trying_to_dereference_null_iterator,
        too_many_tables_specified,
        incorrect_set_fields_specified,
        column_not_found,
        cannot_start_a_transaction_within_a_transaction,
        no_active_transaction,
        incorrect_journal_mode_string,
        incorrect_locking_mode_string,
        invalid_collate_argument_enum,
        failed_to_init_a_backup,
        unknown_member_value,
        incorrect_order,
        cannot_use_default_value,
        arguments_count_does_not_match,
        function_not_found,
        index_is_out_of_bounds,
        value_is_null,
        no_tables_specified,
        empty_range,
    };
}

namespace std {
    template<>
    struct is_error_code_enum<::sqlite_orm::orm_error_code> : true_type {};
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    class orm_error_category : public std::error_category {
      public:
        const char* name() const noexcept override final {
            return "ORM error";
        }

        std::string message(int c) const override final {
            switch (static_cast<orm_error_code>(c)) {
                case orm_error_code::not_found:
                    return "Not found";
                case orm_error_code::type_is_not_mapped_to_storage:
                    return "Type is not mapped to storage";
                case orm_error_code::trying_to_dereference_null_iterator:
                    return "Trying to dereference null iterator";
                case orm_error_code::too_many_tables_specified:
                    return "Too many tables specified";
                case orm_error_code::incorrect_set_fields_specified:
                    return "Incorrect set fields specified";
                case orm_error_code::column_not_found:
                    return "Column not found";
                case orm_error_code::cannot_start_a_transaction_within_a_transaction:
                    return "Cannot start a transaction within a transaction";
                case orm_error_code::no_active_transaction:
                    return "No active transaction";
                case orm_error_code::invalid_collate_argument_enum:
                    return "Invalid collate_argument enum";
                case orm_error_code::failed_to_init_a_backup:
                    return "Failed to init a backup";
                case orm_error_code::unknown_member_value:
                    return "Unknown member value";
                case orm_error_code::incorrect_order:
                    return "Incorrect order";
                case orm_error_code::cannot_use_default_value:
                    return "The statement 'INSERT INTO * DEFAULT VALUES' can be used with only one row";
                case orm_error_code::arguments_count_does_not_match:
                    return "Arguments count does not match";
                case orm_error_code::function_not_found:
                    return "Function not found";
                case orm_error_code::index_is_out_of_bounds:
                    return "Index is out of bounds";
                case orm_error_code::value_is_null:
                    return "Value is null";
                case orm_error_code::no_tables_specified:
                    return "No tables specified";
                default:
                    return "unknown error";
            }
        }
    };

    inline const orm_error_category& get_orm_error_category() {
        static orm_error_category res;
        return res;
    }

    inline std::error_code make_error_code(orm_error_code ev) noexcept {
        return {static_cast<int>(ev), get_orm_error_category()};
    }
}
