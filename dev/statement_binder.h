
#ifndef statement_binder_h
#define statement_binder_h

#include <type_traits>  //  std::is_integral, std::conditional_t, std::enable_if_t, std::is_same
#include <sqlite3.h>    //  sqlite3_stmt, sqlite3_bind_*
#include <vector>
#include <memory>

namespace sqlite_orm {
    
    /**
     *  Helper classes used by statement_binder and row_extractor.
     */
    struct int_or_smaller_tag{};
    struct bigint_tag{};
    struct real_tag{};
    
    template<class V>
    struct arithmetic_tag
    {
        using type = std::conditional_t<
        std::is_integral<V>::value,
        // Integer class
        std::conditional_t<
        sizeof(V) <= sizeof(int),
        int_or_smaller_tag,
        bigint_tag
        >,
        // Floating-point class
        real_tag
        >;
    };
    
    template<class V>
    using arithmetic_tag_t = typename arithmetic_tag<V>::type;
    
    /**
     *  Helper class used for binding fields to sqlite3 statements.
     */
    template<class V, typename Enable = void>
    struct statement_binder
    {
        int bind(sqlite3_stmt *stmt, int index, const V &value);
    };
    
    /**
     *  Specialization for arithmetic types.
     */
    template<class V>
    struct statement_binder<
    V,
    std::enable_if_t<std::is_arithmetic<V>::value>
    >
    {
        int bind(sqlite3_stmt *stmt, int index, const V &value) {
            return bind(stmt, index, value, tag());
        }
        
    private:
        using tag = arithmetic_tag_t<V>;
        
        int bind(sqlite3_stmt *stmt, int index, const V &value, const int_or_smaller_tag&) {
            return sqlite3_bind_int(stmt, index, static_cast<int>(value));
        }
        
        int bind(sqlite3_stmt *stmt, int index, const V &value, const bigint_tag&) {
            return sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
        }
        
        int bind(sqlite3_stmt *stmt, int index, const V &value, const real_tag&) {
            return sqlite3_bind_double(stmt, index, static_cast<double>(value));
        }
    };
    
    
    /**
     *  Specialization for std::string and C-string.
     */
    template<class V>
    struct statement_binder<
    V,
    std::enable_if_t<
    std::is_same<V, std::string>::value
    ||
    std::is_same<V, const char*>::value
    >
    >
    {
        int bind(sqlite3_stmt *stmt, int index, const V &value) {
            return sqlite3_bind_text(stmt, index, string_data(value), -1, SQLITE_TRANSIENT);
        }
        
    private:
        const char* string_data(const std::string& s) const {
            return s.c_str();
        }
        
        const char* string_data(const char* s) const {
            return s;
        }
    };
    
    /**
     *  Specialization for std::nullptr_t.
     */
    template<class V>
    struct statement_binder<
    V,
    std::enable_if_t<std::is_same<V, std::nullptr_t>::value>
    >
    {
        int bind(sqlite3_stmt *stmt, int index, const V &) {
            return sqlite3_bind_null(stmt, index);
        }
    };
    
    /**
     *  Specialization for optional type (std::shared_ptr / std::unique_ptr).
     */
    template <typename T>
    struct is_std_ptr : std::false_type{};
    
    template <typename T>
    struct is_std_ptr<std::shared_ptr<T>> : std::true_type {
        static std::shared_ptr<T> make(const T& v) {
            return std::make_shared<T>(v);
        }
    };
    
    template <typename T>
    struct is_std_ptr<std::unique_ptr<T>> : std::true_type {
        static std::unique_ptr<T> make(const T& v) {
            return std::make_unique<T>(v);
        }
    };
    
    template<class V>
    struct statement_binder<
    V,
    std::enable_if_t<is_std_ptr<V>::value>
    >
    {
        using value_type = typename V::element_type;
        
        int bind(sqlite3_stmt *stmt, int index, const V &value) {
            if(value){
                return statement_binder<value_type>().bind(stmt, index, *value);
            }else{
                return statement_binder<std::nullptr_t>().bind(stmt, index, nullptr);
            }
        }
    };
    
    /**
     *  Specialization for blob type (std::vector<char>).
     */
    template<class V>
    struct statement_binder<
    V,
    std::enable_if_t<std::is_same<V, std::vector<char>>::value>
    >
    {
        int bind(sqlite3_stmt *stmt, int index, const V &value) {
            if (value.size()) {
                return sqlite3_bind_blob(stmt, index, (const void *)&value.front(), int(value.size()), SQLITE_TRANSIENT);
            }else{
                return sqlite3_bind_blob(stmt, index, "", 0, SQLITE_TRANSIENT);
            }
        }
    };
}

#endif /* statement_binder_h */
