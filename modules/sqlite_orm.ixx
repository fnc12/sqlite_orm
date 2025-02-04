// In a module-file, the optional `module;` must appear first; see [cpp.pre].
module;

#define BUILD_SQLITE_ORM_MODULE
#define SQLITE_ORM_IMPORT_STD_MODULE

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
// Including assert.h when using `import std;` below doesn't work (as it leads to multiple defined symbols), hence we include it here
#include <assert.h>  //  assert macro
#endif

export module sqlite_orm;

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
import std.compat;
#endif

#pragma warning(push)
#pragma warning(disable : 5244)  // '#include <filename>' in the purview of module 'sqlite_orm' appears erroneous.

#include <sqlite_orm/sqlite_orm.h>

#pragma warning(pop)
