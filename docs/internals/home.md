# Internals

Documentation of sqlite_orm's internal structure, for contributors and for AI coding
tools working in this repository. None of this is public API — it describes how the
library is built, not how it is used.

## Topics

* [The DSL vocabulary layer](vocabulary-layer.md) — how the internal headers are layered,
  and where a new trait, predicate or algorithm belongs. Start here before adding one.

## Design records

[`docs/plans/`](../plans/) holds dated design documents for individual pieces of work.
Those record the reasoning behind a specific change at a specific time; the topics above
describe how the codebase works now.
