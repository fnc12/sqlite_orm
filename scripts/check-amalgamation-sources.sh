#!/usr/bin/env bash
# Checks that the includes of not_single_header_include/sqlite_orm/sqlite_orm.h and the `sources` of
# third_party/amalgamate/sqlite_orm.json list the same headers in the same order.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

header=not_single_header_include/sqlite_orm/sqlite_orm.h
config=third_party/amalgamate/sqlite_orm.json

if ! diff -u --label "$header" --label "$config" \
    <(sed -nE 's|^#include "\.\./\.\./(dev/[^"]+)"|\1|p' "$header") \
    <(grep -oE '"dev/[^"]+"' "$config" | tr -d '"'); then
    echo "error: $header and $config list different headers or a different order" >&2
    exit 1
fi
