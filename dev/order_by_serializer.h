#pragma once

#include <string>  //  std::string
#include <sstream>  //  std::stringstream

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct order_by_serializer;

        template<class T, class Ctx>
        std::string serialize_order_by(const T& t, const Ctx& context) {
            order_by_serializer<T> serializer;
            return serializer(t, context);
        }

        template<class O>
        struct order_by_serializer<order_by_t<O>, void> {
            using statement_type = order_by_t<O>;

            template<class Ctx>
            std::string operator()(const statement_type& orderBy, const Ctx& context) const {
                std::stringstream ss;
                auto newContext = context;
                newContext.skip_table_name = false;

                ss << serialize(orderBy.expression, newContext);
                if(!orderBy._collate_argument.empty()) {
                    ss << " COLLATE " << orderBy._collate_argument;
                }
                switch(orderBy.asc_desc) {
                    case 1:
                        ss << " ASC";
                        break;
                    case -1:
                        ss << " DESC";
                        break;
                }
                return ss.str();
            }
        };

        template<class C>
        struct order_by_serializer<dynamic_order_by_t<C>, void> {
            using statement_type = dynamic_order_by_t<C>;

            template<class Ctx>
            std::string operator()(const statement_type& orderBy, const Ctx&) const {
                std::stringstream ss;
                ss << static_cast<std::string>(orderBy) << " ";
                int index = 0;
                for(const dynamic_order_by_entry_t& entry: orderBy) {
                    if(index > 0) {
                        ss << ", ";
                    }

                    ss << entry.name;
                    if(!entry._collate_argument.empty()) {
                        ss << " COLLATE " << entry._collate_argument;
                    }
                    switch(entry.asc_desc) {
                        case 1:
                            ss << " ASC";
                            break;
                        case -1:
                            ss << " DESC";
                            break;
                    }
                    ++index;
                };
                return ss.str();
            }
        };

    }
}
