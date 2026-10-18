# Provides the named module `sqlite_orm` as static library target `sqlite_orm_module`,
# where the toolchain provides the standard library modules.
# The target is excluded from the default build target, and is built only for targets that link to it.
#
# Consuming the standard library as C++ named module is a C++23 standard library feature and additionally requires support from build systems:
# - Visual C++ has an option that instructs MSBuild to automatically provision the standard modules,
#   and this option is ON by default when using the `/std:c++latest` compiler switch:
#   "Build ISO C++23 Standard Library Modules", MSBuild property `BuildStlModules`.
# - CMake 4.5 provides the standard modules with the Ninja generators, for GCC 15 (GCC 16 on macOS), Clang 18.1.2 and MSVC 14.36 or newer
#   [see section "import std Support" of the `cmake-cxxmodules` manual].
#   The standard library's module manifest can be overridden with `CMAKE_CXX_STDLIB_MODULES_JSON`,
#   e.g. for the broken `libstdc++.modules.json` shipped by Ubuntu prior to 26.04.
set(SQLITE_ORM_MODULE_INTERFACE_UNIT "")
if(CMAKE_CXX_STANDARD GREATER_EQUAL 23)
    if(CMAKE_GENERATOR MATCHES "^Visual Studio")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL 1930 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.28.2)
            set(SQLITE_ORM_MODULE_INTERFACE_UNIT sqlite_orm.ixx)
        endif()
    elseif(CMAKE_GENERATOR MATCHES "Ninja" AND CMAKE_VERSION VERSION_GREATER_EQUAL 4.5)
        if(APPLE)
            set(SQLITE_ORM_GCC_IMPORT_STD_VERSION 16)
        else()
            set(SQLITE_ORM_GCC_IMPORT_STD_VERSION 15)
        endif()
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL 1936)
            set(SQLITE_ORM_MODULE_INTERFACE_UNIT sqlite_orm.ixx)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL ${SQLITE_ORM_GCC_IMPORT_STD_VERSION})
            set(SQLITE_ORM_MODULE_INTERFACE_UNIT sqlite_orm.cxx)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 18.1.2)
            set(SQLITE_ORM_MODULE_INTERFACE_UNIT sqlite_orm.cppm)
        endif()
    endif()
endif()

if(SQLITE_ORM_MODULE_INTERFACE_UNIT)
    message(STATUS "SQLITE_ORM: Named module from modules/${SQLITE_ORM_MODULE_INTERFACE_UNIT}")

    add_library(sqlite_orm_module STATIC EXCLUDE_FROM_ALL)

    target_sources(sqlite_orm_module
        PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS ${PROJECT_SOURCE_DIR}/modules
        FILES ${PROJECT_SOURCE_DIR}/modules/${SQLITE_ORM_MODULE_INTERFACE_UNIT}
    )

    target_compile_features(sqlite_orm_module PUBLIC cxx_std_23)

    # note: CMake provides the standard library modules only to targets that are scanned for imports by policy CMP0155,
    # which the minimum required CMake version leaves unset, even if their sources are in a module file set
    set_target_properties(sqlite_orm_module PROPERTIES
        CXX_SCAN_FOR_MODULES ON)

    # note: the compile options that affect the module's content need to match those of the importing sources
    if(SQLITE_ORM_OMITS_CODECVT)
        target_compile_definitions(sqlite_orm_module PUBLIC SQLITE_ORM_OMITS_CODECVT=1)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16 AND CMAKE_CXX_STANDARD GREATER_EQUAL 26)
        target_compile_options(sqlite_orm_module PUBLIC -freflection)
    endif()

    target_link_libraries(sqlite_orm_module PUBLIC sqlite_orm)
endif()
