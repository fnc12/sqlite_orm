
#ifndef constraints_h
#define constraints_h

#include <string>
#include <sstream>
#include <type_traits>
#include <sqlite3.h>    //  SQLITE_VERSION_NUMBER

#include "type_printer.h"

namespace sqlite_orm {
    
    namespace internal {
        
        namespace constraints {
            
            /**
             *  AUTOINCREMENT constraint class.
             */
            struct autoincrement_t {
                
                operator std::string() const {
                    return "AUTOINCREMENT";
                }
            };
            
            /**
             *  PRIMARY KEY constraint class.
             *  Cs is parameter pack which contains columns (member pointer and/or function pointers). Can be empty when used withen `make_column` function.
             */
            template<class ...Cs>
            struct primary_key_t {
                std::tuple<Cs...> columns;
                enum class order_by {
                    unspecified,
                    ascending,
                    descending,
                };
                order_by asc_option = order_by::unspecified;
                
                primary_key_t(decltype(columns) c):columns(std::move(c)){}
                
                typedef void field_type;    //  for column iteration. Better be deleted
                typedef std::tuple<> constraints_type;
                
                operator std::string() const {
                    std::string res = "PRIMARY KEY";
                    switch(this->asc_option){
                        case order_by::ascending:
                            res += " ASC";
                            break;
                        case order_by::descending:
                            res += " DESC";
                            break;
                        default:
                            break;
                    }
                    return res;
                }
                
                primary_key_t<Cs...> asc() const {
                    auto res = *this;
                    res.asc_option = order_by::ascending;
                    return res;
                }
                
                primary_key_t<Cs...> desc() const {
                    auto res = *this;
                    res.asc_option = order_by::descending;
                    return res;
                }
            };
            
            /**
             *  UNIQUE constraint class.
             */
            struct unique_t {
                
                operator std::string() const {
                    return "UNIQUE";
                }
            };
            
            /**
             *  DEFAULT constraint class.
             *  T is a value type.
             */
            template<class T>
            struct default_t {
                typedef T value_type;
                
                value_type value;
                
                operator std::string() const {
                    std::stringstream ss;
                    ss << "DEFAULT ";
                    auto needQuotes = std::is_base_of<text_printer, type_printer<T>>::value;
                    if(needQuotes){
                        ss << "'";
                    }
                    ss << this->value;
                    if(needQuotes){
                        ss << "'";
                    }
                    return ss.str();
                }
            };
            
#if SQLITE_VERSION_NUMBER >= 3006019
            
            /**
             *  FOREIGN KEY constraint class.
             *  C is column which has foreign key
             *  R is column which C references to
             *  Available in SQLite 3.6.19 or higher
             */
            template<class C, class R>
            struct foreign_key_t {
                C m = nullptr;
                R r = nullptr;
                
                foreign_key_t(C m_, R r_):m(m_),r(r_){}
                
                typedef void field_type;    //  for column iteration. Better be deleted
                typedef std::tuple<> constraints_type;
                
                template<class L>
                void for_each_column(L) {}
                
                template<class ...Opts>
                constexpr bool has_every() const  {
                    return false;
                }
            };
            
            /**
             *  C can be a class member pointer, a getter function member pointer or setter
             *  func member pointer
             *  Available in SQLite 3.6.19 or higher
             */
            template<class C>
            struct foreign_key_intermediate_t {
                C m = nullptr;
                
                foreign_key_intermediate_t(C m_):m(m_){}
                
                template<class O, class F>
                foreign_key_t<C, F O::*> references(F O::*r) {
                    using ret_type = foreign_key_t<C, F O::*>;
                    return ret_type(this->m, r);
                }
                
                template<class O, class F>
                foreign_key_t<C, const F& (O::*)() const> references(const F& (O::*getter)() const) {
                    using ret_type = foreign_key_t<C, const F& (O::*)() const>;
                    return ret_type(this->m, getter);
                }
                
                template<class O, class F>
                foreign_key_t<C, void (O::*)(F)> references(void (O::*setter)(F)) {
                    using ret_type = foreign_key_t<C, void (O::*)(F)>;
                    return ret_type(this->m, setter);
                }
            };
#endif
            
            struct collate_t {
                internal::collate_argument argument;
                
                collate_t(internal::collate_argument argument_):argument(argument_){}
                
                operator std::string() const {
                    std::string res = "COLLATE ";
                    switch(this->argument){
                        case decltype(this->argument)::binary:
                            res += "BINARY";
                            break;
                        case decltype(this->argument)::nocase:
                            res += "NOCASE";
                            break;
                        case decltype(this->argument)::rtrim:
                            res += "RTRIM";
                            break;
                    }
                    return res;
                }
            };
        }
        
        template<class T>
        struct is_foreign_key : std::false_type{};
        
        template<class C, class R>
        struct is_foreign_key<constraints::foreign_key_t<C, R>> : std::true_type{};
        
        template<class T>
        struct is_primary_key : public std::false_type {};
        
        template<class ...Cs>
        struct is_primary_key<constraints::primary_key_t<Cs...>> : public std::true_type {};
    }
}

#endif /* constraints_h */
