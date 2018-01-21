//
//  sqlite_type.h
//  CPPTest
//
//  Created by John Zakharov on 20.01.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#ifndef sqlite_type_h
#define sqlite_type_h

#include <regex>
#include <map>
#include <string>

namespace sqlite_orm {
    
    namespace internal {
        
        enum class sqlite_type {
            INTEGER,
            TEXT,
            BLOB,
            REAL,
        };
        
        /**
         *  @param str case doesn't matter - it is uppercased before comparing.
         */
        inline std::shared_ptr<sqlite_type> to_sqlite_type(const std::string &str) {
            auto asciiStringToUpper = [](std::string &s){
                std::transform(s.begin(),
                               s.end(),
                               s.begin(),
                               [](char c){
                                   return std::toupper(c);
                               });
            };
            auto upperStr = str;
            asciiStringToUpper(upperStr);
            
            static std::map<sqlite_type, std::vector<std::regex>> typeMap = {
                { sqlite_type::INTEGER, {
                    std::regex("INT"),
                    std::regex("INT.*"),
                    std::regex("TINYINT"),
                    std::regex("SMALLINT"),
                    std::regex("MEDIUMINT"),
                    std::regex("BIGINT"),
                    std::regex("UNSIGNED BIG INT"),
                    std::regex("INT2"),
                    std::regex("INT8"),
                } }, { sqlite_type::TEXT, {
                    std::regex("CHARACTER\\([[:digit:]]+\\)"),
                    std::regex("VARCHAR\\([[:digit:]]+\\)"),
                    std::regex("VARYING CHARACTER\\([[:digit:]]+\\)"),
                    std::regex("NCHAR\\([[:digit:]]+\\)"),
                    std::regex("NATIVE CHARACTER\\([[:digit:]]+\\)"),
                    std::regex("NVARCHAR\\([[:digit:]]+\\)"),
                    std::regex("CLOB"),
                    std::regex("TEXT"),
                } }, { sqlite_type::BLOB, {
                    std::regex("BLOB"),
                } }, { sqlite_type::REAL, {
                    std::regex("REAL"),
                    std::regex("DOUBLE"),
                    std::regex("DOUBLE PRECISION"),
                    std::regex("FLOAT"),
                    std::regex("NUMERIC"),
                    std::regex("DECIMAL\\([[:digit:]]+,[[:digit:]]+\\)"),
                    std::regex("BOOLEAN"),
                    std::regex("DATE"),
                    std::regex("DATETIME"),
                } },
            };
            for(auto &p : typeMap) {
                for(auto &r : p.second) {
                    if(std::regex_match(upperStr, r)){
                        return std::make_shared<sqlite_type>(p.first);
                    }
                }
            }
            
            return {};
        }
        
    }
}

#endif /* sqlite_type_h */
