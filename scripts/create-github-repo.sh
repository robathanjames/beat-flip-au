#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
cd "$project_dir"
command -v gh >/dev/null || { echo "Install GitHub CLI: https://cli.github.com/" >&2; exit 1; }
gh auth status >/dev/null 2>&1 || { echo "Run gh auth login --web first, then rerun this script." >&2; exit 1; }
owner="$(gh api user --jq .login)"
repository_name="${1:-beat-flip-au}"
[[ "$repository_name" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] || { echo "Invalid repository name." >&2; exit 1; }
if ! git rev-parse --git-dir >/dev/null 2>&1; then
  git init -b main
fi
[[ "$(git rev-parse --show-toplevel)" == "$project_dir" ]] || { echo "Use an extracted copy outside another Git repository." >&2; exit 1; }
if git remote get-url origin >/dev/null 2>&1; then
  echo "This folder already has an origin remote; it will not be changed." >&2
  exit 1
fi
git add CMakeLists.txt README.md LICENSE-NOTES.md Source Tests Tools Examples scripts docs .github .gitignore
if ! git diff --cached --quiet; then
  git commit -m "Add Beat Flip AU drum glitch prototype"
fi
gh repo create "$owner/$repository_name" --private --source=. --remote=origin --push \
  --description "Beat Flip: a one-button tempo-synced drum glitch Audio Unit for Logic Pro"
echo "https://github.com/$owner/$repository_name"
