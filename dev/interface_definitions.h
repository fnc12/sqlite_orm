#pragma once

/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializer_context -> db_objects_tuple -> base_table -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */

#include "implementations/column_definitions.h"
#include "implementations/table_definitions.h"
#include "implementations/storage_definitions.h"
