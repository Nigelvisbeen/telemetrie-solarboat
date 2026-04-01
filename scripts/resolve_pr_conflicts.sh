#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./scripts/resolve_pr_conflicts.sh <target-branch> [remote]
# Example:
#   ./scripts/resolve_pr_conflicts.sh main origin

TARGET_BRANCH="${1:-main}"
REMOTE="${2:-origin}"
CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"

if ! git rev-parse --verify "${CURRENT_BRANCH}" >/dev/null 2>&1; then
  echo "Current branch not found" >&2
  exit 1
fi

# Fetch target branch if remote exists.
if git remote get-url "${REMOTE}" >/dev/null 2>&1; then
  git fetch "${REMOTE}" "${TARGET_BRANCH}"
  TARGET_REF="${REMOTE}/${TARGET_BRANCH}"
else
  TARGET_REF="${TARGET_BRANCH}"
fi

echo "Merging ${TARGET_REF} into ${CURRENT_BRANCH}..."
set +e
git merge --no-ff "${TARGET_REF}"
MERGE_EXIT=$?
set -e

if [[ ${MERGE_EXIT} -ne 0 ]]; then
  # If merge failed before creating conflict state (e.g. missing target ref), stop here.
  if [[ ! -f .git/MERGE_HEAD ]] && ! git diff --name-only --diff-filter=U | grep -q .; then
    echo "Merge failed before conflict resolution (check target branch/ref)." >&2
    exit ${MERGE_EXIT}
  fi
fi

if [[ ${MERGE_EXIT} -eq 0 ]]; then
  echo "Merge finished without conflicts."
  exit 0
fi

echo "Conflicts detected. Resolving known files by keeping current branch versions..."

KNOWN_FILES=(
  "README.md"
  "firmware/receiver_esp32_lora/receiver_esp32_lora.ino"
  "firmware/sender_esp32_lora/sender_esp32_lora.ino"
)

for f in "${KNOWN_FILES[@]}"; do
  if git ls-files -u -- "$f" | grep -q .; then
    git checkout --ours -- "$f"
    git add "$f"
    echo "Resolved with ours: $f"
  fi
done

# If other conflicts remain, stop and let user handle manually.
if git diff --name-only --diff-filter=U | grep -q .; then
  echo "Unresolved conflicts remain:" >&2
  git diff --name-only --diff-filter=U >&2
  echo "Please resolve these files manually, then run: git commit" >&2
  exit 2
fi

git commit -m "Resolve PR merge conflicts in telemetry docs and firmware files"
echo "Conflict resolution commit created."
