// In a module-file, the optional `module;` must appear first; see [cpp.pre].
module;

#define BUILD_SQLITE_ORM_MODULE
#define SQLITE_ORM_IMPORT_STD_MODULE

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
// Including assert.h when using `import std;` doesn't work with msvc (as it leads to multiple defined symbols) (see sqlite_orm.ixx), hence we include it here
#include <assert.h>  //  assert macro
#endif

// Non-modular headers that sqlite_orm includes: in the module purview, their declarations would be attached to
// module `sqlite_orm` and conflict with those of the standard library modules.
#include <version>
#include <sqlite3.h>

export module sqlite_orm;

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
import std.compat;
#endif

#include <sqlite_orm/sqlite_orm.h>
