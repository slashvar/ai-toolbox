#!/bin/sh
# Install the pr-self-review skill by symlinking skill/ into ~/.claude/skills/pr-self-review.
#
# Idempotent: replaces an existing symlink at the target, but refuses to overwrite a real
# directory (to avoid clobbering a hand-maintained install).

set -eu

script_dir=$(cd "$(dirname "$0")" && pwd -P)
source_dir="$script_dir/skill"
target_parent="$HOME/.claude/skills"
target="$target_parent/pr-self-review"

if [ ! -d "$source_dir" ]; then
    echo "error: $source_dir does not exist" >&2
    exit 1
fi

mkdir -p "$target_parent"

if [ -L "$target" ]; then
    rm "$target"
elif [ -e "$target" ]; then
    echo "error: $target exists and is not a symlink." >&2
    echo "remove or move it first, then re-run this script." >&2
    exit 1
fi

ln -s "$source_dir" "$target"
echo "installed: $target -> $source_dir"
