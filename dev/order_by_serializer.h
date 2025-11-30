#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <array>
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <utility>  //  std::exchange
#endif

#include "functional/gsl.h"

namespace sqlite_orm::internal {
    template<class T, class SFINAE = void>
    struct order_by_serializer;

    template<class T, class Ctx>
    std::string serialize_order_by(const T& t, const Ctx& context) {
        order_by_serializer<T> serializer;
        return serializer(t, context);
    }

    inline void seralize_collate(std::ostream& ss, const order_by_base& orderBy) {
        if (!orderBy._collate_argument.empty()) {
            ss << " COLLATE " << orderBy._collate_argument;
        }
        switch (orderBy._order) {
            case 1:
                ss << " ASC";
                break;
            case -1:
                ss << " DESC";
                break;
        }
    }

    template<class O>
    struct order_by_serializer<order_by_t<O>, void> {
        using statement_type = order_by_t<O>;

        template<class Ctx>
        SQLITE_ORM_STATIC_CALLOP std::string operator()(const statement_type& orderBy,
                                                        const Ctx& context) SQLITE_ORM_OR_CONST_CALLOP {
            std::stringstream ss;
            auto newContext = context;
            newContext.omit_table_name = false;

            ss << serialize(orderBy._expression, newContext);
            seralize_collate(ss, orderBy);
            return ss.str();
        }
    };

    template<class C>
    struct order_by_serializer<dynamic_order_by_t<C>, void> {
        using statement_type = dynamic_order_by_t<C>;

        template<class Ctx>
        SQLITE_ORM_STATIC_CALLOP std::string operator()(const statement_type& orderBy,
                                                        const Ctx&) SQLITE_ORM_OR_CONST_CALLOP {
            std::stringstream ss;
            ss << static_cast<std::string>(orderBy) << " ";
            static constexpr std::array<orm_gsl::czstring, 2> sep = {", ", ""};
#ifdef SQLITE_ORM_INITSTMT_RANGE_BASED_FOR_SUPPORTED
            for (bool first = true; const dynamic_order_by_entry_t& entry: orderBy) {
                ss << sep[std::exchange(first, false)] << entry.name;
                seralize_collate(ss, entry);
            }
#else
            bool first = true;
            for (const dynamic_order_by_entry_t& entry: orderBy) {
                ss << sep[std::exchange(first, false)] << entry.name;
                seralize_collate(ss, entry);
            }
#endif
            return ss.str();
        }
    };
}
