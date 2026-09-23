#!/bin/sh
set -eu

failed=0
if command -v py >/dev/null 2>&1; then
    PYTHON=py
elif command -v python3 >/dev/null 2>&1; then
    PYTHON=python3
elif command -v python >/dev/null 2>&1; then
    PYTHON=python
else
    printf '%s\n' "Test ledger rejected: Python is required to validate JSON records." >&2
    exit 1
fi

for test_file in $(find tests -type f ! -name README.md -print 2>/dev/null); do
    base=$(basename "$test_file")
    base=${base%.*}
    driver="test-drivers/$base.driver.json"
    result="test-results/$base.json"
    if [ ! -f "$driver" ] || [ ! -f "$result" ]; then
        printf '%s\n' "Test ledger rejected: $test_file requires $driver and $result" >&2
        failed=1
        continue
    fi
    "$PYTHON" - "$driver" "$result" <<'PY' || failed=1
import json
import sys

with open(sys.argv[1], encoding="utf-8") as f:
    driver = json.load(f)
with open(sys.argv[2], encoding="utf-8") as f:
    result = json.load(f)

for key in ("testId", "command", "fixtures"):
    if not driver.get(key):
        raise SystemExit(f"missing driver field: {key}")
for key in ("testId", "status", "commit", "runner", "timestamp", "fixtures"):
    if not result.get(key):
        raise SystemExit(f"missing result field: {key}")
if result["status"] != "pass":
    raise SystemExit("result status must be pass")
if result["testId"] != driver["testId"]:
    raise SystemExit("driver/result testId mismatch")
for record in driver["fixtures"] + result["fixtures"]:
    if not record.get("path") or not record.get("sha256"):
        raise SystemExit("every fixture needs path and sha256")
PY
done

exit "$failed"
