#pragma once

/** @file The tuple of database objects making up a schema.
 *
 *  A schema is represented as a tuple of database objects - tables, views, virtual tables,
 *  indexes and triggers. These are the generic facilities for recognizing and passing around
 *  such a tuple, and unpacking the tuples of database objects passed to `make_storage()`;
 *  locating an individual database object within one is the subject of `algorithms/table_lookup.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::true_type, std::false_type, std::negation, std::disjunction, std::conditional_t
#include <tuple>  //  std::tuple, std::get, std::tuple_size_v, std::tuple_element_t, std::tuple_cat
#include <utility>  //  std::index_sequence, std::make_index_sequence, std::forward, std::move
#endif

#include "../functional/type_traits.h"
#include "../functional/index_sequence_util.h"
#include "../functional/mpl.h"
#include "../tuple_helper/tuple_filter.h"  //  tuple_from_index_sequence_t
#include "../tuple_helper/tuple_traits.h"  //  tuple_has_type
#include "../tuple_helper/tuple_transformer.h"  //  create_from_tuple
#include "../vocabulary/traits/grammar_traits_fwd.h"  //  is_with_clause

namespace sqlite_orm::internal {
    template<class... DBO>
    using db_objects_tuple = std::tuple<DBO...>;

    template<class T>
    struct is_db_objects : std::false_type {};

    template<class... DBO>
    struct is_db_objects<std::tuple<DBO...>> : std::true_type {};
    // note: cannot use `db_objects_tuple` alias template because older compilers have problems
    // to match `const db_objects_tuple`.
    template<class... DBO>
    struct is_db_objects<const std::tuple<DBO...>> : std::true_type {};

    /**
     *  Quoted trait metafunction that checks whether a type is a tuple of database objects containing type `T`.
     */
    template<class T>
    using check_if_bundles = mpl::conjunction<check_if<is_db_objects>, check_if_has_type<T>>;

    /**
     *  Whether the database object at index `I` of the tuple at index `K` of arguments `Specs` is kept:
     *  it is dropped if an argument is of the same type, or if an earlier tuple or an earlier element of the same tuple
     *  is of the same type.
     */
    template<size_t K,
             size_t I,
             class Specs,
             class Bundle = std::tuple_element_t<K, Specs>,
             class DBO = std::tuple_element_t<I, Bundle>>
    struct is_unique_bundled_dbo
        : std::negation<std::disjunction<
              // an argument is of type `DBO`
              tuple_has_type<Specs, DBO>,
              // one of the arguments [0, K) is a tuple containing `DBO`
              mpl::invoke_t<mpl::contains<check_if_bundles<DBO>>,
                            tuple_from_index_sequence_t<Specs, std::make_index_sequence<K>>>,
              // one of the elements [0, I) of the tuple is of type `DBO`
              tuple_has_type<tuple_from_index_sequence_t<Bundle, std::make_index_sequence<I>>, DBO>>> {};

    /**
     *  The index sequence of the kept database objects of the tuple at index `K` of arguments `Specs`.
     */
    template<size_t K, class Specs, class Seq>
    struct bundled_dbo_index_sequence;

    template<size_t K, class Specs, size_t... Idx>
    struct bundled_dbo_index_sequence<K, Specs, std::index_sequence<Idx...>>
        // index `Idx` of the tuple if kept, nothing otherwise
        : flatten_idxseq<std::conditional_t<is_unique_bundled_dbo<K, Idx, Specs>::value,
                                            std::index_sequence<Idx>,
                                            std::index_sequence<>>...> {};

    /**
     *  Unpack the argument at index `K` of arguments `Specs`: a tuple of database objects into its kept
     *  database objects, any other argument into a tuple of itself.
     */
    template<size_t K, class Specs, class Spec>
    auto unpack_db_objects(Spec spec) {
        if constexpr (is_db_objects<Spec>::value) {
            using index_sequence =
                typename bundled_dbo_index_sequence<K, Specs, std::make_index_sequence<std::tuple_size<Spec>::value>>::
                    type;
            return create_from_tuple<std::tuple>(std::move(spec), index_sequence{});
        } else {
            return std::tuple<Spec>{std::move(spec)};
        }
    }

    /**
     *  Unpack the tuples of database objects among arguments `Spec...`, keeping one database object per type:
     *  a bundled database object is dropped if an argument is of the same type, or if it is bundled once more
     *  in an earlier position.
     */
    template<class... Spec, size_t... K>
    auto unpack_db_objects(std::tuple<Spec...> specs, std::index_sequence<K...>) {
        return std::tuple_cat(unpack_db_objects<K, std::tuple<Spec...>>(std::get<K>(std::move(specs)))...);
    }

#ifdef SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
    /**
     *  Concatenate newly created tables with given DBOs, forming a new set of DBOs.
     */
    template<typename DBOs, typename... Tables>
    auto db_objects_cat(const DBOs& dbObjects, Tables&&... tables) {
        auto& [... elements] = dbObjects;
        return std::tuple{std::forward<Tables>(tables)..., elements...};
    }
#else
    template<typename DBOs, size_t... Idx, typename... Tables>
    auto db_objects_cat(const DBOs& dbObjects, std::index_sequence<Idx...>, Tables&&... tables) {
        return std::tuple{std::forward<Tables>(tables)..., std::get<Idx>(dbObjects)...};
    }

    /**
     *  Concatenate newly created tables with given DBOs, forming a new set of DBOs.
     */
    template<typename DBOs, typename... Tables>
    auto db_objects_cat(const DBOs& dbObjects, Tables&&... tables) {
        return db_objects_cat(dbObjects,
                              std::make_index_sequence<std::tuple_size_v<DBOs>>{},
                              std::forward<Tables>(tables)...);
    }
#endif

    /**
     *  Return passed in DBOs.
     *
     *  Note: A WITH clause prepends the database objects of its CTEs, see `cte_db_objects.h`.
     */
    template<class DBOs,
             class E,
             std::enable_if_t<std::conjunction_v<is_db_objects<DBOs>, std::negation<is_with_clause<E>>>, bool> = true>
    decltype(auto) db_objects_for_expression(DBOs& dbObjects, const E&) {
        return dbObjects;
    }
}
