
#ifndef row_extractor_h
#define row_extractor_h

#include <sqlite3.h>
#include <type_traits>
#include <cstdlib>
#include <tuple>

namespace sqlite_orm {
    
    /**
     *  Helper class used to cast values from argv to V class
     *  which depends from column type.
     *
     */
    template<class V, typename Enable = void>
    struct row_extractor{
        //  used in sqlite3_exec (select)
        V extract(const char *row_value);
        
        //  used in sqlite_column (iteration, get_all)
        V extract(sqlite3_stmt *stmt, int columnIndex);
    };
    
    /**
     *  Specialization for arithmetic types.
     */
    template<class V>
    struct row_extractor<
    V,
    std::enable_if_t<std::is_arithmetic<V>::value>
    >
    {
        V extract(const char *row_value) {
            return extract(row_value, tag());
        }
        
        V extract(sqlite3_stmt *stmt, int columnIndex) {
            return extract(stmt, columnIndex, tag());
        }
        
    private:
        using tag = arithmetic_tag_t<V>;
        
        V extract(const char *row_value, const int_or_smaller_tag& trait) {
            return static_cast<V>(atoi(row_value));
        }
        
        V extract(sqlite3_stmt *stmt, int columnIndex, const int_or_smaller_tag& trait) {
            return static_cast<V>(sqlite3_column_int(stmt, columnIndex));
        }
        
        V extract(const char *row_value, const bigint_tag& trait) {
            return static_cast<V>(atoll(row_value));
        }
        
        V extract(sqlite3_stmt *stmt, int columnIndex, const bigint_tag& trait) {
            return static_cast<V>(sqlite3_column_int64(stmt, columnIndex));
        }
        
        V extract(const char *row_value, const real_tag& trait) {
            return static_cast<V>(atof(row_value));
        }
        
        V extract(sqlite3_stmt *stmt, int columnIndex, const real_tag& trait) {
            return static_cast<V>(sqlite3_column_double(stmt, columnIndex));
        }
    };
    
    /**
     *  Specialization for std::string.
     */
    template<class V>
    struct row_extractor<
    V,
    std::enable_if_t<std::is_same<V, std::string>::value>
    >
    {
        std::string extract(const char *row_value) {
            if(row_value){
                return row_value;
            }else{
                return {};
            }
        }
        
        std::string extract(sqlite3_stmt *stmt, int columnIndex) {
            auto cStr = (const char*)sqlite3_column_text(stmt, columnIndex);
            if(cStr){
                return cStr;
            }else{
                return {};
            }
        }
    };
    
    /**
     *  Specialization for std::vector<char>.
     */
    template<class V>
    struct row_extractor<
    V,
    std::enable_if_t<std::is_same<V, std::vector<char>>::value>
    >
    {
        std::vector<char> extract(const char *row_value) {
            if(row_value){
                auto len = ::strlen(row_value);
                return this->go(row_value, static_cast<int>(len));
            }else{
                return {};
            }
        }
        
        std::vector<char> extract(sqlite3_stmt *stmt, int columnIndex) {
            auto bytes = static_cast<const char *>(sqlite3_column_blob(stmt, columnIndex));
            auto len = sqlite3_column_bytes(stmt, columnIndex);
            return this->go(bytes, len);
        }
        
    protected:
        
        std::vector<char> go(const char *bytes, int len) {
            if(len){
                std::vector<char> res;
                res.reserve(len);
                std::copy(bytes,
                          bytes + len,
                          std::back_inserter(res));
                return res;
            }else{
                return {};
            }
        }
    };
    
    template<class V>
    struct row_extractor<
    V,
    std::enable_if_t<is_std_ptr<V>::value>
    >
    {
        using value_type = typename V::element_type;
        
        V extract(const char *row_value) {
            if(row_value){
                return is_std_ptr<V>::make(row_extractor<value_type>().extract(row_value));
            }else{
                return {};
            }
        }
        
        V extract(sqlite3_stmt *stmt, int columnIndex) {
            auto type = sqlite3_column_type(stmt, columnIndex);
            if(type != SQLITE_NULL){
                return is_std_ptr<V>::make(row_extractor<value_type>().extract(stmt, columnIndex));
            }else{
                return {};
            }
        }
    };
    
    /**
     *  Specialization for std::vector<char>.
     */
    template<>
    struct row_extractor<std::vector<char>> {
        std::vector<char> extract(const char *row_value) {
            if(row_value){
                auto len = ::strlen(row_value);
                return this->go(row_value, static_cast<int>(len));
            }else{
                return {};
            }
        }
        
        std::vector<char> extract(sqlite3_stmt *stmt, int columnIndex) {
            auto bytes = static_cast<const char *>(sqlite3_column_blob(stmt, columnIndex));
            auto len = sqlite3_column_bytes(stmt, columnIndex);
            return this->go(bytes, len);
        }
        
    protected:
        
        std::vector<char> go(const char *bytes, int len) {
            if(len){
                std::vector<char> res;
                res.reserve(len);
                std::copy(bytes,
                          bytes + len,
                          std::back_inserter(res));
                return res;
            }else{
                return {};
            }
        }
    };
    
    template<class ...Args>
    struct row_extractor<std::tuple<Args...>> {
        
        std::tuple<Args...> extract(char **argv) {
            std::tuple<Args...> res;
            this->extract<std::tuple_size<decltype(res)>::value>(res, argv);
            return res;
        }
        
        std::tuple<Args...> extract(sqlite3_stmt *stmt, int columnIndex) {
            (void)columnIndex;
            std::tuple<Args...> res;
            this->extract<std::tuple_size<decltype(res)>::value>(res, stmt);
            return res;
        }
        
    protected:
        
        template<size_t I, typename std::enable_if<I != 0>::type * = nullptr>
        void extract(std::tuple<Args...> &t, sqlite3_stmt *stmt) {
            typedef typename std::tuple_element<I - 1, typename std::tuple<Args...>>::type tuple_type;
            std::get<I - 1>(t) = row_extractor<tuple_type>().extract(stmt, I - 1);
            this->extract<I - 1>(t, stmt);
        }
        
        template<size_t I, typename std::enable_if<I == 0>::type * = nullptr>
        void extract(std::tuple<Args...> &, sqlite3_stmt *) {
            //..
        }
        
        template<size_t I, typename std::enable_if<I != 0>::type * = nullptr>
        void extract(std::tuple<Args...> &t, char **argv) {
            typedef typename std::tuple_element<I - 1, typename std::tuple<Args...>>::type tuple_type;
            std::get<I - 1>(t) = row_extractor<tuple_type>().extract(argv[I - 1]);
            this->extract<I - 1>(t, argv);
        }
        
        template<size_t I, typename std::enable_if<I == 0>::type * = nullptr>
        void extract(std::tuple<Args...> &, char **) {
            //..
        }
    };
}

#endif /* row_extractor_h */
