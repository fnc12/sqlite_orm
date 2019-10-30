#pragma once

#include <sstream>
#include <string>  //  std::string

#include "core_functions.h"
#include "constraints.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct statement_serializator {
            using statement_type = T;

            std::string operator()(const statement_type &t) const {
                std::stringstream ss;
                ss << t;
                return ss.str();
            }
        };

        template<class T>
        std::string serialize(const T &t) {
            statement_serializator<T> serializator;
            return serializator(t);
        }

        template<class R, class S, class... Args>
        struct statement_serializator<core_functions::core_function_t<R, S, Args...>, void> {
            using statement_type = core_functions::core_function_t<R, S, Args...>;

            std::string operator()(const statement_type &c) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << "(";
                std::vector<std::string> args;
                using args_type = typename std::decay<decltype(c)>::type::args_type;
                args.reserve(std::tuple_size<args_type>::value);
                iterate_tuple(c.args, [&args](auto &v) {
                    args.push_back(serialize(v));
                });
                for(size_t i = 0; i < args.size(); ++i) {
                    ss << args[i];
                    if(i < args.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")";
                return ss.str();
            }
        };

        template<>
        struct statement_serializator<constraints::autoincrement_t, void> {
            using statement_type = constraints::autoincrement_t;

            std::string operator()(const statement_type &c) const {
                return static_cast<std::string>(c);
            }
        };

        template<class... Cs>
        struct statement_serializator<constraints::primary_key_t<Cs...>, void> {
            using statement_type = constraints::primary_key_t<Cs...>;

            std::string operator()(const statement_type &c) const {
                return static_cast<std::string>(c);
            }
        };

        template<>
        struct statement_serializator<constraints::unique_t, void> {
            using statement_type = constraints::unique_t;

            std::string operator()(const statement_type &c) const {
                return static_cast<std::string>(c);
            }
        };
        
        template<>
        struct statement_serializator<constraints::collate_t, void> {
            using statement_type = constraints::collate_t;
            
            std::string operator()(const statement_type &c) const {
                return static_cast<std::string>(c);
            }
        };

        template<class T>
        struct statement_serializator<constraints::default_t<T>, void> {
            using statement_type = constraints::default_t<T>;

            std::string operator()(const statement_type &c) const {
                return static_cast<std::string>(c) + " (" + serialize(c.value) + ")";
            }
        };

        template<>
        struct statement_serializator<std::string, void> {
            using statement_type = std::string;

            std::string operator()(const statement_type &c) const {
                return "\"" + c + "\"";
            }
        };

        template<>
        struct statement_serializator<const char *, void> {
            using statement_type = const char *;

            std::string operator()(const char *c) const {
                return std::string("'") + c + "'";
            }
        };

    }
}
