#pragma once

#include <sqlite3.h>
#include <system_error>  //  std::system_error
#include <string>  //  std::string
#include <memory>
#include <utility>  //  std::move, std::exchange

#include "error_code.h"
#include "connection_holder.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  A backup class. Don't construct it as is, call storage.make_backup_from or storage.make_backup_to instead.
         *  An instance of this class represents a wrapper around sqlite3_backup pointer. Use this class
         *  to have maximum control on a backup operation. In case you need a single backup in one line you
         *  can skip creating a backup_t instance and just call storage.backup_from or storage.backup_to function.
         */
        struct backup_t {
            backup_t(connection_ref to_,
                     const std::string& zDestName,
                     connection_ref from_,
                     const std::string& zSourceName,
                     std::unique_ptr<connection_holder> holder_) :
                handle(sqlite3_backup_init(to_.get(), zDestName.c_str(), from_.get(), zSourceName.c_str())),
                holder(move(holder_)), to(to_), from(from_) {
                if(!this->handle) {
                    throw std::system_error{orm_error_code::failed_to_init_a_backup};
                }
            }

            backup_t(backup_t&& other) :
                handle(std::exchange(other.handle, nullptr)), holder(move(other.holder)), to(other.to),
                from(other.from) {}

            ~backup_t() {
                if(this->handle) {
                    (void)sqlite3_backup_finish(this->handle);
                }
            }

            /**
             *  Calls sqlite3_backup_step with pages argument
             */
            int step(int pages) {
                return sqlite3_backup_step(this->handle, pages);
            }

            /**
             *  Returns sqlite3_backup_remaining result
             */
            int remaining() const {
                return sqlite3_backup_remaining(this->handle);
            }

            /**
             *  Returns sqlite3_backup_pagecount result
             */
            int pagecount() const {
                return sqlite3_backup_pagecount(this->handle);
            }

          protected:
            sqlite3_backup* handle = nullptr;
            std::unique_ptr<connection_holder> holder;
            connection_ref to;
            connection_ref from;
        };
    }
}
