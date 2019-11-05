#pragma once

#include <string>   //  std::string
#include <sqlite3.h>
#include <system_error> //  std::error_code, std::system_error

#include "error_code.h"

namespace sqlite_orm {
    
    namespace internal {

        inline sqlite3 *open_db(std::string const&filename)
    	{
    		sqlite3 *result {nullptr};
			if(sqlite3_open(filename.c_str(), &result) != SQLITE_OK){
				
				throw std::system_error(std::error_code(sqlite3_errcode(result), get_sqlite_error_category()), get_error_message(result, "opening '", filename, "'. "));
            }
			return result;
    	}
    	
        struct database_connection {
            explicit database_connection(const std::string &filename):
				db {open_db(filename) }
    		{
            }
            
            ~database_connection() {
                sqlite3_close(this->db);
            }
            
            sqlite3* get_db() {
                return this->db;
            }
            
        protected:
            sqlite3 *db;
        };
    }
}
