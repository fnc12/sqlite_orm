#pragma once

#include <string>  //  std::string
#include <tuple>  //  std::ignore
#include <sqlite_orm/sqlite_orm.h>
#include <ostream>  //  std::ostream

namespace PreparedStatementTests {
    struct User {
        int id = 0;
        std::string name;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        friend bool operator==(const User&, const User&) = default;
#else
        friend bool operator==(const User& lhs, const User& rhs) {
            return lhs.id == rhs.id && lhs.name == rhs.name;
        }

        friend bool operator!=(const User& lhs, const User& rhs) {
            return !(lhs == rhs);
        }
#endif
    };

    struct Visit {
        int id = 0;
        decltype(User::id) userId = 0;
        long time = 0;
    };

    struct UserAndVisit {
        decltype(User::id) userId = 0;
        decltype(Visit::id) visitId = 0;
        std::string description;
    };

    inline void testSerializing(const sqlite_orm::internal::prepared_statement_base& statement) {
        auto sql = statement.sql();
        std::ignore = sql;
#if SQLITE_VERSION_NUMBER >= 3014000
        auto expanded = statement.expanded_sql();
        std::ignore = expanded;
#endif
#if SQLITE_VERSION_NUMBER >= 3026000 and defined(SQLITE_ENABLE_NORMALIZE)
        auto normalized = statement.normalized_sql();
        std::ignore = normalized;
#endif
    }

    inline std::ostream& operator<<(std::ostream& os, const User& user) {
        return os << "{" << user.id << ", " << user.name << "}";
    }
}
