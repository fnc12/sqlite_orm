#pragma once

#include <utility>  //  std::forward, std::move

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/tuple.h"

namespace sqlite_orm {
    namespace internal {
#if SQLITE_VERSION_NUMBER >= 3024000
        template<class T, class A>
        struct upsert_clause;

        template<class... Args>
        struct conflict_target {
            using args_tuple = mpl::tuple<Args...>;

            args_tuple args;

            upsert_clause<args_tuple, mpl::tuple<>> do_nothing() {
                return {std::move(this->args), {}};
            }

            template<class... ActionsArgs>
            upsert_clause<args_tuple, mpl::tuple<ActionsArgs...>> do_update(ActionsArgs... actions) {
                return {std::move(this->args), {std::forward<ActionsArgs>(actions)...}};
            }
        };

        template<class... TargetArgs, class... ActionsArgs>
        struct upsert_clause<mpl::tuple<TargetArgs...>, mpl::tuple<ActionsArgs...>> {
            using target_args_tuple = mpl::tuple<TargetArgs...>;
            using actions_tuple = mpl::tuple<ActionsArgs...>;

            target_args_tuple target_args;

            actions_tuple actions;
        };

        template<class T>
        using is_upsert_clause = polyfill::is_specialization_of<T, upsert_clause>;
    }

    /**
     *  ON CONFLICT upsert clause builder function.
     *  @example
     *  storage.insert(into<Employee>(),
     *            columns(&Employee::id, &Employee::name, &Employee::age, &Employee::address, &Employee::salary),
     *            values(std::make_tuple(3, "Sofia", 26, "Madrid", 15000.0),
     *                 std::make_tuple(4, "Doja", 26, "LA", 25000.0)),
     *            on_conflict(&Employee::id).do_update(set(c(&Employee::name) = excluded(&Employee::name),
     *                                           c(&Employee::age) = excluded(&Employee::age),
     *                                           c(&Employee::address) = excluded(&Employee::address),
     *                                           c(&Employee::salary) = excluded(&Employee::salary))));
     */
    template<class... Args>
    internal::conflict_target<Args...> on_conflict(Args... args) {
        return {{std::forward<Args>(args)...}};
    }
#endif
}
