---
name: move-header
description: Use when moving, renaming, splitting or dissolving a header under sqlite_orm's dev/ folder. Covers deciding the destination, git mv, fixing includes, the include manifests, the two header variants, the documentation and the amalgamation.
---

# Moving or renaming a header

## 1. Decide the destination

- The folder follows `docs/internals/source-layout.md`; traits and algorithms follow the placement
  test of `docs/internals/vocabulary-layer.md`. If the move changes what a folder holds, update its
  entry in the source layout in the same change.
- Check the open work in `docs/internals/vocabulary-layer.md` and `TODO.md` for a move that is
  already planned or decided differently.

## 2. Move it

- `git mv` the file, so that history follows it. When splitting, move the original to the part
  keeping most of its content, and create the others.
- After a `git mv`, never list the old path in a later `git add`; stage by the new path.
- Update the header's `@file` comment if it describes its location or content.

## 3. Fix every reference

Search for the old path and its basename (`grep -rn 'old_name.h'`) in:

- `#include`s across `dev/`, `tests/` and `examples/`. Includes are relative to the including
  header; keep each include's trailing comment naming what it is included for.
- `dev/node_definitions.h`, `dev/node_algorithm_definitions.h` and `dev/interface_definitions.h`,
  if the header is registered there.
- `not_single_header_include/sqlite_orm/sqlite_orm.h` and `third_party/amalgamate/config.json`, if it
  is a top-level header: both, in the same order.
- `AGENTS.md`, `docs/internals/`, `.claude/agents/` and `.claude/skills/`. `docs/plans/` records
  past designs and is not updated.

A moved header must not gain a guarded inclusion on the way: guard contents, never an `#include`.

## 4. Check

Run `scripts/pre-commit.sh`. It regenerates the amalgamation, whose content must not change beyond
the moved code's position, and checks the order of the amalgamation sources and the documented
paths. Then have the change reviewed against `docs/internals/review-checklist.md`.

A pure move goes into a commit of its own, separate from changes to the moved code, so that
`git log --follow` and review see the move as a rename.
