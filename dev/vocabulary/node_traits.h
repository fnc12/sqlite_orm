#pragma once

/**	@file Umbrella header for the single-node vocabulary: the open classification traits,
 *        and the projections they are read with.
 *
 *  It deliberately spans two folders, because both answer a question about a node already in
 *  hand - what it is, and what it carries - whereas `node_algorithms.h` covers the composed
 *  judgments that follow from them.
 */

#include "traits/grammar_traits_fwd.h"
#include "traits/structural_traits_fwd.h"
#include "traits/semantic_traits_fwd.h"
#include "traits/operand_traits_fwd.h"
#include "projections/nested_types.h"
#include "projections/mapped_types.h"
