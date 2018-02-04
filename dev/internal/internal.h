
#ifndef internal_h
#define internal_h

#include <sqlite3.h>    //  sqlite3_stmt, sqlite3_finalize, sqlite3_open, SQLITE_OK, sqlite3_errmsg, sqlite3_close, sqlite3
#include <string>   //  std::string
#include <stdexcept>    //  std::runtime_error
#include <type_traits>  //  std::forward

namespace sqlite_orm {
    
    namespace internal {
        
        template<class T>
        struct distinct_t {
            T t;
            
            operator std::string() const {
                return "DISTINCT";
            }
        };
        
        template<class T>
        struct all_t {
            T t;
            
            operator std::string() const {
                return "ALL";
            }
        };
        
        template<class ...Args>
        struct columns_t {
            bool distinct = false;
            
            template<class L>
            void for_each(L) {
                //..
            }
            
            int count() {
                return 0;
            }
        };
        
        template<class T, class ...Args>
        struct columns_t<T, Args...> : public columns_t<Args...> {
            T m;
            
            columns_t(decltype(m) m_, Args&& ...args): super(std::forward<Args>(args)...), m(m_) {}
            
            template<class L>
            void for_each(L l) {
                l(this->m);
                this->super::for_each(l);
            }
            
            int count() {
                return 1 + this->super::count();
            }
        private:
            using super = columns_t<Args...>;
        };
        
        template<class ...Args>
        struct set_t {
            
            operator std::string() const {
                return "SET";
            }
            
            template<class F>
            void for_each(F) {
                //..
            }
        };
        
        template<class L, class R, class ...Args>
        struct set_t<L, R, Args...> : public set_t<Args...> {
            L l;
            R r;
            
            using super = set_t<Args...>;
            
            set_t(L l_, R r_, Args&& ...args) : super(std::forward<Args>(args)...), l(std::move(l_)), r(std::forward<R>(r_)) {}
            
            template<class F>
            void for_each(F f) {
                f(l, r);
                this->super::for_each(f);
            }
        };
        
        struct database_connection {
            
            database_connection(const std::string &filename) {
                auto rc = sqlite3_open(filename.c_str(), &this->db);
                if(rc != SQLITE_OK){
                    auto msg = sqlite3_errmsg(this->db);
                    throw std::runtime_error(msg);
                }
            }
            
            ~database_connection() {
                sqlite3_close(this->db);
            }
            
            sqlite3* get_db() {
                return this->db;
            }
            
        protected:
            sqlite3 *db = nullptr;
        };
        
        /**
         *  Trait class used to define table mapped type by setter/getter/member
         */
        template<class T>
        struct table_type;
        
        template<class O, class F>
        struct table_type<F O::*> {
            using type = O;
        };
        
        template<class O, class F>
        struct table_type<const F& (O::*)() const> {
            using type = O;
        };
        
        template<class O, class F>
        struct table_type<void (O::*)(F)> {
            using type = O;
        };
        
        struct statement_finalizer {
            sqlite3_stmt *stmt = nullptr;
            
            statement_finalizer(decltype(stmt) stmt_):stmt(stmt_){}
            
            inline ~statement_finalizer() {
                sqlite3_finalize(this->stmt);
            }
            
        };
        
        /**
         *  Result of concatenation || operator
         */
        template<class L, class R>
        struct conc_t {
            L l;
            R r;
        };
        
        template<class T>
        struct expression_t {
            T t;
            
            expression_t(T t_):t(t_){}
        };
    }
}

#endif /* internal_h */
