# Troubleshooting

Common errors when running `reg-def-gen`, their causes, and how to fix them.

## Stage 2/3 — Generation errors

### `Wrote /path/to/...RegDef.h (... chars, 0 typedefs)`

The input JSON contains registers that all failed validation, or all were
deduplicated away.  Either:

- Check the JSON is an object with a `registers` array.
- Verify each register dict has `meta.offset` set; missing-offsets are dropped.
- Verify the JSON parses: `python3 -c "import json; json.load(open('regs.json'))"`.

### `typedef union {...}` missing

The earlier versions of this skill (DDR-specific scripts in `/tmp/`) emitted
typedefs only for the FIRST iteration of a duplicate.  The current
IP-agnostic generator deduplicates by `(section, name, offset)`.  If you
have a deliberate shadow with the same name in two chapters, give each a
distinct offset.

## Stage 4 — Validation errors

### `struct != 32`

The most common cause is **overlapping bit ranges** in the input JSON:

    {"bits": "31:24", ...},   // 8-bit field
    {"bits": "27:24", ...},   // overlap

Both fields claim bits 27:24, the struct sums to 32 + 4 = 36 bits.  Fix the
input JSON by removing one of the overlapping fields or splitting.

The validator identifies the affected register by name (`U_<IP>_<REG>`).

### `struct != u32`

The SV header has an `int unsigned u32 = {...}` initializer whose widths
don't match the struct.  This indicates a bug in the generator's
`fill_field_gaps` function (rare).  Re-run from a clean checkout.

### `brace balance: 5 { / 4 }`

A field description contains a curly brace without a matching close.  The
description is concatenated into a comment, so any unescaped `{` / `}` in
the databook text will break the C parser.

**Fix:** in the source databook text, replace stray braces with their
Unicode escapes (`&#123;` / `&#125;`) or simply remove them.

### `Asymmetric _REG_OFFSET macros` / `_REG_ADDR`

Both C and SV headers contain different sets of macro names.  Possible
causes:

- The JSON was filtered after the C header was generated and before the SV
  one was.
- A typo / case mismatch in the IP prefix (`--ip-prefix dw_apb_wdt` vs
  `--ip-prefix DW_apb_wdt`).  Macros emit `DW_APB_WDT_*` regardless of
  input case in `gen_sv_header.py`, but `gen_c_header.py` preserves the
  case.  Pass the same prefix both times.

## Pre-flight checklist

Before running `run_all.sh`:

| Check | How |
|-------|-----|
| JSON has at least one register | `jq '.registers | length' regs.json` |
| Every register has `meta.offset` | `jq '.registers[] | select(.meta.offset == null)' regs.json` |
| No two fields in the same register share any bit | the validator will report this |
| All fields with bits `"x:N"` resolve to a reasonable width | run `python3 resolve_field_width.py` (self-tests) |
| IP prefix matches the project naming | the macro tier (`<IP>_REG_OFFSET`) uses the upper-case prefix |

## When variables don't resolve

If the resolver returns `None` (and the field ends up at the 4-bit
default), inspect the input:

    python3 - <<'PY'
import json
data = json.load(open("regs.json"))
for r in data["registers"]:
    for f in r["fields"]:
        if "x" in f["bits"]:
            print(r["name"], f["bits"], "::", f["desc"].split("Range Variable[x]:", 1)[-1].strip().splitlines()[0])
PY

Update `PARAM_MAX` in `scripts/resolve_field_width.py` for any new parameter.

## When validate gives a non-zero exit code

`run_all.sh` exits non-zero if the validator reports any errors.  To see the
errors only, run:

```bash
$SKILL_DIR/scripts/validate_headers.py \
    --c-header /tmp/out/RegDef.h \
    --sv-header /tmp/out/RegDef.svh
```

The validator writes to stderr but exits with code 0 on success and 1 on
failure.  CI pipelines can use this directly.

## Recovering after a partial run

Delete the partial output and re-run:

```bash
rm -f ${OUT_DIR}/${PREFIX}RegDef.h ${OUT_DIR}/${PREFIX}RegDef.svh
$SKILL_DIR/scripts/run_all.sh --json ... --base-addr ... --out-dir ${OUT_DIR} \
    --ip-prefix ${PREFIX}
```
