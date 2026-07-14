# Variable-Width Field Resolver

## Background

Synopsys databooks express the maximum width of fields like
`active_ranks`, `rfc_tmgreg_sel`, etc. as a small expression embedded in
the field description after the marker

    Range Variable[x]:   <EXPR>[ +/- OFFSET]

`<EXPR>` may be a single parameter, a parameter with a fixed +/- offset, or a
ternary on a configuration parameter.  The resolver computes the **maximum**
legal value across all valid IP configurations, then the **width** is

    width = (max_value [+ OFFSET]) - LO + 1

`LO` comes from the original bit position (e.g. `x:24` -> LO = 24).

## Supported grammar

| Expression form                                  | Example                                            | Computed max |
|--------------------------------------------------|----------------------------------------------------|--------------|
| Bare parameter                                   | `"MEMC_NUM_RANKS"`                                 | PARAM_MAX lookup |
| Param + integer                                  | `"MEMC_NUM_RANKS" + 7`                             | PARAM_MAX[MEMC_NUM_RANKS] + 7 |
| Param - integer                                  | `"MEMC_NUM_RANKS" - 1`                             | PARAM_MAX[MEMC_NUM_RANKS] - 1 |
| Inner expression (paren)                         | `"MEMC_WRCMD_ENTRY_BITS - 1"`                      | re-evaluated |
| Inner expression with trailing offset            | `"MEMC_WRCMD_ENTRY_BITS - 1" + 7`                  | max(inner) + 7 |
| Ternary on cfg param                             | `"(MEMC_NUM_RANKS==2) ? 2 : 4"`                    | max of all numeric/param branches |
| Nested ternary                                   | `"(c1==1) ? 1 : (c2==2) ? 4 : 2"`                  | max of all numeric/param branches |
| Quoted expression with trailing offset            | `"(cond) ? 2 : 4" + 23`                            | max of branches + 23 |

Single-quoted trailing offset patterns like `"EXPR"\n- 1` (the marker can be
followed by line wraps) are normalised before evaluation.

## PARAM_MAX coverage

`scripts/resolve_field_width.py` carries a curated dictionary:

```python
PARAM_MAX = {
    # ranks / ports / channels
    'MEMC_NUM_RANKS': 4, 'MEMC_NUM_RANK': 4, 'UMCTL2_NUM_PORTS': 4,
    # address-map
    'MEMC_ROW_ADDR_WIDTH': 18, 'MEMC_COL_ADDR_WIDTH': 12,
    'MEMC_BANK_ADDR_WIDTH': 4, 'MEMC_CS_ADDR_WIDTH': 2,
    'MEMC_BG_BITS': 2, 'MEMC_BANK_BITS': 2, 'MEMC_RANK_BITS': 2,
    'MEMC_PAGE_BITS': 18, 'MEMC_HIF_ADDR_WIDTH_MAX': 40,
    # DRAM data
    'MEMC_DRAM_DATA_WIDTH': 32,
    # VCID / BSM / queue
    'UMCTL2_CID_WIDTH': 3, 'UMCTL2_BSM_BITS': 6,
    'MEMC_RDCMD_ENTRY_BITS': 6, 'MEMC_WRCMD_ENTRY_BITS': 6,
    # ECC / retry / scrub / parity
    'MEMC_MAX_INLINE_ECC_PER_BURST_BITS': 2,
    'UMCTL2_RETRY_MAX_ADD_RD_LAT_LG2': 4, 'UMCTL2_DATARAM_PAR_DW_LG2': 4,
    'UMCTL2_DATARAM_PAR_DW': 4,
    'UMCTL2_OCPAR_WDATA_OUT_ERR_WIDTH': 4, 'UMCTL2_OCPAR_ADDR_LOG_HIGH_WIDTH': 4,
    'UMCTL2_REG_SCRUB_INTERVALW': 4,
    '_DEFAULT': 4,
}
```

Unknown parameters fall back to 4 (`_DEFAULT`).  When the resolver cannot
evaluate an expression (malformed input, unknown function, unbalanced
parens, ...), `resolve_field_width` returns `None` and the field is emitted
at width=4 (the conservative default).

## Adding a new parameter

If you process an IP with a parameter that isn't in `PARAM_MAX`, the field will
silently use width=4 - which is wrong.  Extend the dictionary:

```python
PARAM_MAX['NEW_PARAM'] = <max legal value>
```

Then run the resolver's built-in self-tests:

```bash
$ python3 scripts/resolve_field_width.py
... 25 passed, 0 failed
```

If you have a real failing case, add it to the `cases = [...]` list at the
bottom of the resolver and re-run.

## Edge cases and pitfalls

1. **Ternary max.**  Use "max of branches", NOT "max of conditions".  The
   resolver picks the largest integer (or quoted-param value) it sees in
   the RHS, ignoring the predicate.

2. **\"1\" + N pattern.**  A leading constant inside quotes looks like a
   parameter reference; the resolver recognises both forms.

3. **Multi-line marker.**  Databooks sometimes wrap the trailing offset
   onto the next line: `"EXPR"\n+ 23`.  The regex tolerates whitespace
   between the closing quote and the sign.

4. **Default fallback.**  When the resolver gives up, the resulting struct
   field is still emitted at width 4.  The validator (Stage 4) will catch
   the resulting struct==32 mismatch and flag it.

## Self-tests

25 test cases are baked into the module.  Run them with:

```bash
$ python3 scripts/resolve_field_width.py
```
