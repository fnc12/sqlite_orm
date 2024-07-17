// In a module-file, the optional `module;` must appear first; see [cpp.pre].
module;

#define _BUILD_SQLITE_ORM_MODULE
#define _IMPORT_STD_MODULE

#ifdef _IMPORT_STD_MODULE
// Including assert.h when using `import std;` below doesn't work, so we include it here
#include <assert.h>  //  assert macro
#endif

export module sqlite_orm;

#ifdef _IMPORT_STD_MODULE
import std;
#endif

#pragma warning(push)
#pragma warning(disable : 5244)  // '#include <meow>' in the purview of module 'sqlite_orm' appears erroneous.

#include <sqlite_orm/sqlite_orm.h>

#pragma warning(pop)
