#!/usr/bin/env bash
# Prepares the working tree for a commit: regenerates the amalgamations, formats the sources with
# clang-format 20 (the version CI lints with), warns about changed files containing carriage returns, and checks
# the order of the amalgamation sources (scripts/check-amalgamation-sources.sh) and the paths named in the
# documentation (scripts/check-doc-paths.sh).
#
# Runs on Linux, macOS and WSL, from any directory.
# Environment: CLANG_FORMAT (default clang-format-20), PYTHON (default python3).

set -euo pipefail
shopt -s globstar nullglob

cd "$(dirname "${BASH_SOURCE[0]}")/.."

clang_format="${CLANG_FORMAT:-clang-format-20}"
python="${PYTHON:-python3}"

version="$("$clang_format" --version | sed -nE 's/.*version ([0-9]+).*/\1/p')"
if [[ "$version" != 20 ]]; then
    echo "error: $clang_format is version ${version:-unknown}, CI lints with clang-format 20; set CLANG_FORMAT" >&2
    exit 1
fi

echo "Regenerating the amalgamation"
"$python" third_party/amalgamate/amalgamate.py -c third_party/amalgamate/sqlite_orm.json -s .
"$python" third_party/amalgamate/amalgamate.py -c third_party/amalgamate/sqlite_orm_config.json -s .

echo "Formatting the sources"
"$clang_format" --Wno-error=unknown -i -style=file \
    include/**/*.h dev/**/*.h tests/**/*.h tests/**/*.cpp examples/**/*.cpp

crlf_files=()
while IFS= read -r -d '' path; do
    if [[ -f "$path" ]] && grep -Iq $'\r' "$path"; then
        crlf_files+=("$path")
    fi
done < <(git ls-files -z --modified --others --exclude-standard)
if ((${#crlf_files[@]})); then
    echo "warning: changed files with carriage returns, the repository uses LF line endings:" >&2
    printf '  %s\n' "${crlf_files[@]}" >&2
fi

echo "Checking the order of the amalgamation sources"
scripts/check-amalgamation-sources.sh

echo "Checking the paths named in the documentation"
scripts/check-doc-paths.sh

git status --short
