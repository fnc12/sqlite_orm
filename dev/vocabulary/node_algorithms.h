#pragma once

/** @file
 *  Purpose: gather what follows from what a node or a field type is and carries.
 *
 *  Intention: the umbrella of `algorithms/` in the vocabulary layer, whose rules `docs/internals/vocabulary-layer.md`
 *  holds; the consumers in `dev/` include it in place of the files behind it.
 */

#include "algorithms/accessors.h"
#include "algorithms/argument_placeholders.h"
#include "algorithms/clause_predicates.h"
#include "algorithms/column_fields.h"
#include "algorithms/ddl_predicates.h"
#include "algorithms/expression_element_predicates.h"
#include "algorithms/field_predicates_concepts.h"
#include "algorithms/field_predicates_fwd.h"
#include "algorithms/index_filters.h"
#include "algorithms/operand_predicates.h"
#include "algorithms/recordset_predicates.h"
