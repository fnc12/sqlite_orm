// In a module-file, the optional `module;` must appear first; see [cpp.pre].
module;

#define BUILD_SQLITE_ORM_MODULE
#define SQLITE_ORM_IMPORT_STD_MODULE

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
// Including assert.h when using `import std;` doesn't work with msvc (as it leads to multiple defined symbols) (see sqlite_orm.ixx), hence we include it here
#include <assert.h>  //  assert macro
#endif

// Non-modular headers that sqlite_orm includes and that declare entities, kept in the global module;
// their inclusions in the module purview are then skipped by their include guards.
#include <version>
#include <sqlite3.h>

export module sqlite_orm;

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
import std.compat;
#endif

#pragma clang diagnostic push
// warning: '#include <filename>' attaches the declarations to the named module 'sqlite_orm', which is not usually intended; consider moving that directive before the module declaration
#pragma clang diagnostic ignored "-Winclude-angled-in-module-purview"

#include <sqlite_orm/sqlite_orm.h>

#pragma clang diagnostic pop
