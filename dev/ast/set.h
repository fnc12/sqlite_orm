#pragma once

#include <tuple>  //  std::tuple, std::tuple_size
#include <string>  //  std::string
#include <vector>  //  std::vector
#include <sstream>  //  std::stringstream

namespace sqlite_orm {

    namespace internal {

        template<class... Args>
        struct set_t {
            using assigns_type = std::tuple<Args...>;

            assigns_type assigns;
        };

        template<class C>
        struct dynamic_set_t {
            using context_t = C;
            using entry_t = std::string;
            using const_iterator = typename std::vector<entry_t>::const_iterator;

            dynamic_set_t(const context_t& context_) : context(context_) {}

            template<class L, class R>
            void push_back(assign_t<L, R> assign) {
                auto newContext = this->context;
                newContext.skip_table_name = true;
                std::stringstream ss;
                ss << serialize(assign.lhs, newContext) << ' ' << assign.serialize() << ' '
                   << serialize(assign.rhs, context);
                entries.push_back(ss.str());
            }

            const_iterator begin() const {
                return this->entries.begin();
            }

            const_iterator end() const {
                return this->entries.end();
            }

            void clear() {
                this->entries.clear();
            }

          protected:
            context_t context;
            std::vector<entry_t> entries;
        };
    }

    /**
     *  SET keyword used in UPDATE ... SET queries.
     *  Args must have `assign_t` type. E.g. set(assign(&User::id, 5)) or set(c(&User::id) = 5)
     */
    template<class... Args>
    internal::set_t<Args...> set(Args... args) {
        using arg_tuple = std::tuple<Args...>;
        static_assert(std::tuple_size<arg_tuple>::value ==
                          internal::count_tuple<arg_tuple, internal::is_assign_t>::value,
                      "set function accepts assign operators only");
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  SET keyword used in UPDATE ... SET queries. It is dynamic version. It means use can add amount of arguments now known at compilation time but known at runtime.
     */
    template<class S>
    internal::dynamic_set_t<internal::serializer_context<typename S::db_objects_type>> dynamic_set(const S& storage) {
        internal::serializer_context_builder<S> builder(storage);
        return builder();
    }
}
