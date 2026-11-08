#pragma once

/** @file
 *  Purpose: gather what a node is and what it carries.
 *
 *  Intention: the umbrella of `traits/` and `projections/` in the vocabulary layer, whose rules
 *  `docs/internals/vocabulary-layer.md` holds; the consumers in `dev/` include it in place of the files behind
 *  it, and the algorithms behind `node_algorithms.h` build on it.
 */

#include "projections/mapped_types.h"
#include "projections/nested_types.h"
#include "traits/grammar_traits_fwd.h"
#include "traits/operand_traits_fwd.h"
#include "traits/recordset_concepts.h"
#include "traits/semantic_traits_fwd.h"
#include "traits/structural_traits_fwd.h"
