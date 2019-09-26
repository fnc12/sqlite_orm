#pragma once

#include <sqlite3.h>
#include <string>   //  std::string
#include <memory>

#include "error_code.h"
#include "connection_holder.h"

namespace sqlite_orm {
    
    namespace internal {
        
        struct backup_t {
            backup_t(connection_ref to_,
                     const std::string &zDestName,
                     connection_ref from_,
                     const std::string &zSourceName,
                     std::unique_ptr<connection_holder> holder_) :
            handle(sqlite3_backup_init(to_.get(), zDestName.c_str(), from_.get(), zSourceName.c_str())),
            holder(move(holder_)),
            to(to_),
            from(from_)
            {
                if(!this->handle){
                    throw std::system_error(std::make_error_code(orm_error_code::failed_to_init_a_backup));
                }
            }
            
            backup_t(backup_t &&other) :
            handle(other.handle),
            holder(move(other.holder)),
            to(other.to),
            from(other.from)
            {
                other.handle = nullptr;
            }
            
            ~backup_t() {
                if(this->handle){
                    (void)sqlite3_backup_finish(this->handle);
                    this->handle = nullptr;
                }
            }
            
            int step(int pages) {
                return sqlite3_backup_step(this->handle, pages);
            }
            
            int remaining() const {
                return sqlite3_backup_remaining(this->handle);
            }
            
            int pagecount() const {
                return sqlite3_backup_pagecount(this->handle);
            }
            
        protected:
            sqlite3_backup *handle = nullptr;
            std::unique_ptr<connection_holder> holder;
            connection_ref to;
            connection_ref from;
        };
    }
}
