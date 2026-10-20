#!/usr/bin/env bash
# Checks that the repository paths named in the agent guidance and the internals documentation exist:
# relative markdown link targets, and backticked paths of at least two segments, taken from the repository
# root, `dev/` or `dev/vocabulary/` (git remote branches excepted). Glob patterns must match at least one file.
#
# Usage: scripts/check-doc-paths.sh [file...]; without arguments the documents listed below are checked.

set -euo pipefail
shopt -s globstar nullglob

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if (($#)); then
    documents=("$@")
else
    documents=(AGENTS.md CLAUDE.md docs/internals/*.md .claude/agents/*.md .claude/skills/**/*.md)
fi

exists() {
    compgen -G "$1" > /dev/null
}

status=0
for document in "${documents[@]}"; do
    directory="$(dirname "$document")"

    while IFS= read -r target; do
        target="${target%%#*}"
        [[ -z "$target" || "$target" =~ ^[a-z]+: ]] && continue
        if ! exists "$directory/$target"; then
            echo "$document: link target '$target' does not exist"
            status=1
        fi
    done < <(grep -oE '\]\([^)[:space:]]+\)' "$document" | sed -E 's/^\]\((.*)\)$/\1/')

    while IFS= read -r path; do
        [[ "${path%/}" == */* && ! "$path" =~ ^(origin|upstream)/ ]] || continue
        if ! exists "$path" && ! exists "dev/$path" && ! exists "dev/vocabulary/$path"; then
            echo "$document: path '$path' does not exist"
            status=1
        fi
    done < <(grep -oE '`[A-Za-z0-9_.*-]+(/[A-Za-z0-9_.*-]+)+/?`' "$document" | tr -d '`')
done

exit $status
