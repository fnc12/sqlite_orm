#pragma once

#include <string>  //  std::string
#include <vector>  //  std::vector
#include <sstream>  //  std::stringstream

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct order_by_serializator;

        template<class T, class C>
        std::string serialize_order_by(const T& t, const C& context) {
            order_by_serializator<T> serializator;
            return serializator(t, context);
        }

        template<class O>
        struct order_by_serializator<order_by_t<O>, void> {
            using statement_type = order_by_t<O>;

            template<class C>
            std::string operator()(const statement_type& orderBy, const C& context) const {
                std::stringstream ss;
                auto newContext = context;
                newContext.skip_table_name = false;
                auto columnName = serialize(orderBy.expression, newContext);
                ss << columnName << " ";
                if(orderBy._collate_argument.length()) {
                    ss << "COLLATE " << orderBy._collate_argument << " ";
                }
                switch(orderBy.asc_desc) {
                    case 1:
                        ss << "ASC";
                        break;
                    case -1:
                        ss << "DESC";
                        break;
                }
                return ss.str();
            }
        };

        template<class S>
        struct order_by_serializator<dynamic_order_by_t<S>, void> {
            using statement_type = dynamic_order_by_t<S>;

            template<class C>
            std::string operator()(const statement_type& orderBy, const C&) const {
                std::vector<std::string> expressions;
                for(auto& entry: orderBy) {
                    std::string entryString;
                    {
                        std::stringstream ss;
                        ss << entry.name << " ";
                        if(!entry._collate_argument.empty()) {
                            ss << "COLLATE " << entry._collate_argument << " ";
                        }
                        switch(entry.asc_desc) {
                            case 1:
                                ss << "ASC";
                                break;
                            case -1:
                                ss << "DESC";
                                break;
                        }
                        entryString = ss.str();
                    }
                    expressions.push_back(move(entryString));
                };
                std::stringstream ss;
                ss << static_cast<std::string>(orderBy) << " ";
                for(size_t i = 0; i < expressions.size(); ++i) {
                    ss << expressions[i];
                    if(i < expressions.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << " ";
                return ss.str();
            }
        };

    }
}
