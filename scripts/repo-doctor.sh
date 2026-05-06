#!/usr/bin/env bash
set -euo pipefail

fail=0

say() {
  printf '%s\n' "$1"
}

bad() {
  say "[FAIL] $1"
  fail=1
}

ok() {
  say "[OK]   $1"
}

say "LOOGAL REPO DOCTOR"
say "=================="

# 1. Make sure we are in a git repo.
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  bad "not inside a git repository"
  exit 1
fi

ok "inside git repository"

# 2. Check tracked/staged/untracked state.
if [ -z "$(git status --porcelain)" ]; then
  ok "working tree clean"
else
  bad "working tree has changes"
  git status --short
fi

# 3. Check untracked non-ignored files.
untracked="$(git ls-files --others --exclude-standard)"
if [ -z "$untracked" ]; then
  ok "no untracked non-ignored files"
else
  bad "untracked non-ignored files found"
  printf '%s\n' "$untracked"
fi

# 4. Check for tracked object files.
tracked_objects="$(git ls-files '*.o')"
if [ -z "$tracked_objects" ]; then
  ok "no tracked .o build artifacts"
else
  bad "tracked .o build artifacts found"
  printf '%s\n' "$tracked_objects"
fi

# 5. Check for suspicious paste junk files.
suspicious=""
for f in "=" "EOF" "PY" "C_EOF" "EOF'" "PY'"; do
  if [ -e "$f" ]; then
    suspicious="${suspicious}${f}"$'\n'
  fi
done

if [ -z "$suspicious" ]; then
  ok "no suspicious root-level paste junk files"
else
  bad "suspicious paste junk files found"
  printf '%s' "$suspicious"
fi

# 6. Check for merge conflict markers in tracked source/docs.
if git grep -n -E '^(<<<<<<< .+|=======$|>>>>>>> .+)' -- \
  '*.c' '*.h' '*.sh' '*.md' 'Makefile' >/tmp/loogal_conflicts.$$ 2>/dev/null; then
  bad "merge conflict markers found"
  cat /tmp/loogal_conflicts.$$
else
  ok "no merge conflict markers found"
fi
rm -f /tmp/loogal_conflicts.$$

# 7. Check whitespace errors in diff.
if git diff --check >/tmp/loogal_diffcheck.$$ 2>&1; then
  ok "git diff whitespace check passed"
else
  bad "git diff whitespace check failed"
  cat /tmp/loogal_diffcheck.$$
fi
rm -f /tmp/loogal_diffcheck.$$

if [ "$fail" -eq 0 ]; then
  say ""
  say "REPO DOCTOR VERDICT: CLEAN"
  exit 0
else
  say ""
  say "REPO DOCTOR VERDICT: POLLUTED OR NEEDS ATTENTION"
  exit 1
fi
