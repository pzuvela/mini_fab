#!/usr/bin/env bash
set -euo pipefail

echo "=== Checking JavaScript syntax ==="

find web/public/js -type f -name '*.js' -print0 |
    while IFS= read -r -d '' file; do
        echo "Checking $file"
        node --check "$file"
    done

echo "=== JavaScript syntax OK ==="
