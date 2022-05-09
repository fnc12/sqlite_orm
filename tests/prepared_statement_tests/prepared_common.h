#pragma once

#include <string>  //  std::string
#include <tuple>  //  std::ignore
#include <sqlite_orm/sqlite_orm.h>
#include <ostream>  //  std::ostream

namespace PreparedStatementTests {
    struct User {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id, std::string name) : id{id}, name{move(name)} {}
#endif
    };

    struct Visit {
        int id = 0;
        decltype(User::id) userId;
        long time = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Visit() = default;
        Visit(int id, decltype(Visit::userId) userId, long time) : id{id}, userId{userId}, time{time} {}
#endif
    };

    struct UserAndVisit {
        decltype(User::id) userId;
        decltype(Visit::id) visitId;
        std::string description;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        UserAndVisit() = default;
        UserAndVisit(decltype(UserAndVisit::userId) userId,
                     decltype(UserAndVisit::visitId) visitId,
                     std::string description) :
            userId{userId},
            visitId{visitId}, description{move(description)} {}
#endif
    };

    inline bool operator==(const User& lhs, const User& rhs) {
        return lhs.id == rhs.id && lhs.name == rhs.name;
    }

    inline bool operator!=(const User& lhs, const User& rhs) {
        return !(lhs == rhs);
    }

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
