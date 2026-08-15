#!/usr/bin/env python3
"""
Fix multi-line comment alignment in sqlite_orm dev/ headers.

For a comment opening at column N:
- Continuation lines with `*` should have `*` at column N+1
- Text on continuation lines should start at the next multiple of 4 from column 0
- Continuation lines without `*` get an asterisk added
- The closing `*/` should be at column N+1

This script adds asterisks to continuation lines that don't have them, making all comments consistent.
"""

import re
from pathlib import Path


def next_multiple_of_4(n):
    """Return the next multiple of 4 that is greater than n."""
    return ((n // 4) + 1) * 4


def fix_comment_alignment(file_path):
    """Fix comment alignment in a single file."""
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    modified = False
    result_lines = []
    in_comment = False
    comment_start_col = 0

    i = 0
    while i < len(lines):
        line = lines[i]

        # Check if we're starting a multi-line comment
        if not in_comment:
            # Look for /* or /** at the start of a line (after whitespace)
            match = re.match(r'^(\s*)/(\*+)(.*)$', line)
            if match:
                leading_spaces = match.group(1)
                stars = match.group(2)
                rest = match.group(3)

                # Check if it's a single-line comment
                if rest.rstrip().endswith('*/'):
                    result_lines.append(line)
                    i += 1
                    continue

                # Multi-line comment starts
                in_comment = True
                comment_start_col = len(leading_spaces)
                result_lines.append(line)
                i += 1
                continue

        # If we're in a comment, process continuation and closing lines
        if in_comment:
            # Expected indentation for continuation lines
            star_col = comment_start_col + 1  # Where * should be
            text_col = next_multiple_of_4(star_col)  # Where text should start (next multiple of 4)

            # Check if this is the closing line (just spaces followed by */)
            if re.match(r'^\s*\*/$', line):
                current_indent = len(line) - len(line.lstrip())
                if current_indent != star_col:
                    fixed_line = ' ' * star_col + '*/\n'
                    result_lines.append(fixed_line)
                    modified = True
                else:
                    result_lines.append(line)
                in_comment = False
                i += 1
                continue

            # Check for continuation line starting with *
            match = re.match(r'^(\s+)\*\s*(.*)$', line)
            if match:
                current_indent = len(match.group(1))
                text_content = match.group(2)

                # Calculate expected format
                spaces_after_star = text_col - star_col - 1  # -1 for the asterisk itself
                expected_line = ' ' * star_col + '*' + ' ' * spaces_after_star + text_content + '\n'

                # Check if current line matches expected format
                if line != expected_line:
                    result_lines.append(expected_line)
                    modified = True
                else:
                    result_lines.append(line)

                # Check if this line contains the closing */
                if text_content.rstrip().endswith('/'):
                    in_comment = False
                i += 1
                continue

            # Continuation line without asterisk
            # Add asterisk and align properly, but skip preprocessor directives
            if line.lstrip().startswith('#'):
                # Preprocessor directive - this ends the comment
                result_lines.append(line)
                in_comment = False
                i += 1
                continue

            if line.strip():  # Non-empty line
                content = line.lstrip()
                # Add asterisk and proper spacing
                spaces_after_star = text_col - star_col - 1
                fixed_line = ' ' * star_col + '*' + ' ' * spaces_after_star + content
                result_lines.append(fixed_line)
                modified = True
            else:
                # Empty line in comment - keep as is
                result_lines.append(line)

            i += 1
            continue

        # Normal line (not in comment)
        result_lines.append(line)
        i += 1

    if modified:
        new_content = ''.join(result_lines)
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False


def main():
    """Process all .h files in dev/ directory."""
    dev_dir = Path('dev')

    if not dev_dir.exists():
        print(f"Error: {dev_dir} directory not found")
        return

    modified_files = []

    # Find all .h files recursively
    for h_file in sorted(dev_dir.rglob('*.h')):
        if fix_comment_alignment(h_file):
            modified_files.append(str(h_file))
            print(f"Fixed: {h_file}")

    print(f"\n{'='*60}")
    print(f"Total files modified: {len(modified_files)}")
    print(f"{'='*60}")

    if modified_files:
        print("\nModified files:")
        for f in modified_files:
            print(f"  - {f}")


if __name__ == '__main__':
    main()
