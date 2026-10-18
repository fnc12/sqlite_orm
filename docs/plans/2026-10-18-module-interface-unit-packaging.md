# Packaging the module interface units

Date: 2026-10-18
Branch base: `feature/public-config-header` (#1598)

## Goal

An installed sqlite_orm ships `<sqlite_orm/sqlite_orm.h>` and `<sqlite_orm/sqlite_orm_config.h>`, but no module
interface unit, so `import sqlite_orm;` works only in a build of the source tree. The installed package should ship
the interface unit, for the consumer to build the module with its own compiler and flags.

## Current state

- `packaging/CMakeLists.txt` installs `FILE_SET CXX_MODULES` of target `sqlite_orm`. That target is an `INTERFACE`
  library without a module file set, so nothing is installed.
- The file set belongs to `sqlite_orm_module` (`cmake/SqliteOrmModule.cmake`), which is neither installed nor
  exported. It is defined only if sqlite_orm is the top-level project.
- `SqliteOrmTargets` is exported twice to the same destination, once with `CXX_MODULES_DIRECTORY` and without
  namespace, once with namespace `sqlite_orm::`.

## Approach

CMake 3.28 installs a module file set with `install(TARGETS ... FILE_SET CXX_MODULES DESTINATION ...)` and records it
in the exported targets with `install(EXPORT ... CXX_MODULES_DIRECTORY ...)`. A CMake consumer then compiles the
interface unit as part of its own build.

- Install and export `sqlite_orm_module` instead of `sqlite_orm`, into one `SqliteOrmTargets` export with namespace
  `sqlite_orm::`. The interface unit keeps going to `${SQLITE_ORM_INSTALL_CMAKEDIR}/src`.
- The interface unit includes `<sqlite_orm/sqlite_orm.h>`, which resolves against the installed include directory.
- Importers include `<sqlite_orm/sqlite_orm_config.h>` before `import sqlite_orm;` to test the configuration.

## Open questions

- `sqlite_orm_module` has to be defined when sqlite_orm is installed, not only as the top-level project.
- `cmake/SqliteOrmModule.cmake` picks one of `sqlite_orm.ixx`, `.cppm` and `.cxx` for the compiler at configure time,
  so the installed package would carry the unit for the installer's compiler only. The units differ only in their
  warning suppressions and comments.
- The consumer's toolchain has to provide the standard library modules: the interface units import `std.compat`.
- The Visual Studio generators don't support exporting or installing module information.

## Out of scope

There is no tool-agnostic install location for module interface units as of CMake 4.5: only the standard libraries
are located via their `modules.json` manifests. The Common Package Specification, exported by CMake 4.3's
`install(PACKAGE_INFO)` (experimental), doesn't cover C++ modules in a release yet; CMake's development branch adds
`CXX_MODULES_DIRECTORY` to it. Revisit once it is released.
