/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializator_context -> storage_impl -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once

#include "implementations/column_definitions.h"
#include "implementations/table_definitions.h"
#include "implementations/storage_impl_definitions.h"
#include "implementations/storage_definitions.h"
