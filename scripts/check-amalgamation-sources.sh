#!/usr/bin/env bash
# Checks that the includes of each header in not_single_header_include/sqlite_orm/ and the `sources` of its
# amalgamation config in third_party/amalgamate/ list the same headers in the same order.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

status=0
while read -r header config; do
    if ! diff -u --label "$header" --label "$config" \
        <(sed -nE 's|^#include "\.\./\.\./(dev/[^"]+)"|\1|p' "$header") \
        <(grep -oE '"dev/[^"]+"' "$config" | tr -d '"'); then
        echo "error: $header and $config list different headers or a different order" >&2
        status=1
    fi
done <<'PAIRS'
not_single_header_include/sqlite_orm/sqlite_orm.h third_party/amalgamate/config.json
not_single_header_include/sqlite_orm/sqlite_orm_config.h third_party/amalgamate/config_header.json
PAIRS
exit $status
