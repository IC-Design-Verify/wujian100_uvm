#!/usr/bin/env bash
# run_all.sh - one-shot pipeline for the reg-def-gen skill.
#
# Reads a parsed JSON (Stage 1 output, see SKILL.md "Input Contract") and
# runs:
#   Stage 2  -> gen_c_header.py          produces <IP>RegDef.h
#   Stage 3  -> gen_sv_header.py         produces <IP>RegDef.svh
#   Stage 4  -> validate_headers.py      exits non-zero if either header is bad
#
# Usage:
#   ./run_all.sh --json <regs.json> --base-addr 0xC0001000 \
#                --out-dir /path/to/header_files --ip-prefix DW_apb_wdt
#
# The IP prefix is normally derived from the JSON (longest common prefix of
# register-name keys).  Pass --ip-prefix explicitly to override.
#
# All paths must be absolute.  Python 3 is required.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
    cat <<EOF
Usage: $0 --json FILE --base-addr HEX --out-dir DIR [--ip-prefix PREFIX]
                [--source-label LABEL] [--skip-validate]
EOF
    exit 2
}

JSON=""
BASE_ADDR=""
OUT_DIR=""
IP_PREFIX=""
SOURCE_LABEL="auto-generated"
SKIP_VALIDATE=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --json)         JSON="$2"; shift 2 ;;
        --base-addr)    BASE_ADDR="$2"; shift 2 ;;
        --out-dir)      OUT_DIR="$2"; shift 2 ;;
        --ip-prefix)    IP_PREFIX="$2"; shift 2 ;;
        --source-label) SOURCE_LABEL="$2"; shift 2 ;;
        --skip-validate) SKIP_VALIDATE=1; shift ;;
        -h|--help)      usage ;;
        *)              echo "Unknown option: $1" >&2; usage ;;
    esac
done

[[ -z "$JSON" ]]      && { echo "ERROR: --json is required" >&2; usage; }
[[ -z "$BASE_ADDR" ]] && { echo "ERROR: --base-addr is required" >&2; usage; }
[[ -z "$OUT_DIR" ]]   && { echo "ERROR: --out-dir is required" >&2; usage; }
[[ ! -f "$JSON" ]]   && { echo "ERROR: --json not found: $JSON" >&2; exit 2; }
[[ -d "$OUT_DIR" ]]   || mkdir -p "$OUT_DIR"

IP_UPPER=$(python3 - <<PY
import json, sys
data = json.load(open("$JSON"))
names = [r["name"] for r in data.get("registers", [])]
parts = names[0].split("_") if names else ["DW","IP"]
common = []
for idx, p in enumerate(parts):
    if all(n.split("_")[idx] == p for n in names if len(n.split("_")) > idx):
        common.append(p)
    else:
        break
print("_".join(common).rstrip("_") if common else "DW_IP")
PY
)
PREFIX=${IP_PREFIX:-$IP_UPPER}
HEADER="${OUT_DIR}/${PREFIX}RegDef.h"
SVH="${OUT_DIR}/${PREFIX}RegDef.svh"

echo ">>> Stage 2: generate C header -> ${HEADER}"
python3 "${SCRIPT_DIR}/gen_c_header.py" \
    --input "$JSON" \
    --output "$HEADER" \
    --base-addr "$BASE_ADDR" \
    --ip-prefix "$PREFIX" \
    --source-label "$SOURCE_LABEL"

echo ">>> Stage 3: generate SV header -> ${SVH}"
python3 "${SCRIPT_DIR}/gen_sv_header.py" \
    --input "$JSON" \
    --output "$SVH" \
    --base-addr "$BASE_ADDR" \
    --ip-prefix "$PREFIX" \
    --source-label "$SOURCE_LABEL"

if [[ "$SKIP_VALIDATE" -eq 0 ]]; then
    echo ">>> Stage 4: validate"
    python3 "${SCRIPT_DIR}/validate_headers.py" \
        --c-header "$HEADER" --sv-header "$SVH"
fi

echo ">>> Done: ${HEADER} and ${SVH}"
