/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializer_context -> storage_impl -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once
#include <sqlite3.h>
#include <algorithm>  //  std::find_if
#include <sstream>  //  std::stringstream
#include <cstdlib>  //  std::atoi

#include "../error_code.h"
#include "../storage_impl.h"
#include "../util.h"

namespace sqlite_orm {
    namespace internal {}
}
