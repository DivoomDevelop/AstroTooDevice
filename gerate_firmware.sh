#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mode=--release
if [[ "${1:-}" == --debug ]]; then
    mode=--debug
    shift
fi
exec bash "$PROJECT_ROOT/build.sh" "$mode" "$@"
