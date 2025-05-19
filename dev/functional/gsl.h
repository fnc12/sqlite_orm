#pragma once

/** @file A subset of the Guidelines Support Library (GSL) as it is useful for this library.
 *  
 *  At the time of writing, the use of these symbols serves only to express the logical intention, because:
 *  1. Each facility lives in the nested namespace `orm_gsl` because `gsl` may conflict with other GSL implementations [like ms-gsl or gsl-lite].
 *  2. Tools like Clang-Tidy are currently hard-wired to the `gsl` namespace to check the guidelines.
 *  3. There is no way to attach an attribute to tell Clang-Tidy that it is a "GSL" symbol.
 *  
 *  This might seem frustrating at first, but things are constantly evolving and need time, and many facilities found their way into the STL.
 *  There's an ongoing discussion about this limitation on the guidelines repository:
 *  https://github.com/isocpp/CppCoreGuidelines/issues/144
 *  https://github.com/isocpp/CppCoreGuidelines/issues/1519
 *  
 *  However, these symbols are very valuable as we can "grep" for them and easily update things in the future.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_pointer
#endif

namespace sqlite_orm {

    // span
    namespace orm_gsl {

        inline constexpr size_t dynamic_extent = size_t(-1);

    }

    // C-style string types
    namespace orm_gsl {
        //
        // These are "tag" typedefs for C-style strings (i.e. null-terminated character arrays)
        // that allow static analysis to help find bugs.
        //
        // There are no additional features/semantics that we can find a way to add inside the
        // type system for these types that will not either incur significant runtime costs or
        // (sometimes needlessly) break existing programs when introduced.
        //

        template<typename Char, size_t Extent = dynamic_extent>
        using basic_zstring = Char*;

        using czstring = basic_zstring<const char, dynamic_extent>;

        using cwzstring = basic_zstring<const wchar_t, dynamic_extent>;

        using cu16zstring = basic_zstring<const char16_t, dynamic_extent>;

        using cu32zstring = basic_zstring<const char32_t, dynamic_extent>;

        using zstring = basic_zstring<char, dynamic_extent>;

        using wzstring = basic_zstring<wchar_t, dynamic_extent>;

        using u16zstring = basic_zstring<char16_t, dynamic_extent>;

        using u32zstring = basic_zstring<char32_t, dynamic_extent>;

    }

    // pointers
    namespace orm_gsl {

        //
        // owner
        //
        // `orm_gsl::owner<T>` is designed as a safety mechanism for code that must deal directly with raw pointers that own memory.
        // Ideally such code should be restricted to the implementation of low-level abstractions.
        //
        // T must be a pointer type
        //
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
        template<class T>
            requires std::is_pointer_v<T>
        using owner = T;
#else
        template<class T, typename = std::enable_if_t<std::is_pointer<T>::value>>
        using owner = T;
#endif

    }
}
