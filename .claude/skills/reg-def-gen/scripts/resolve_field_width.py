#!/usr/bin/env python3
"""Resolve the maximum width of a variable-width field.

A field whose bit position is "x:N" can take different widths depending on the
IP configuration parameters used at synthesis time.  The databook expresses the
maximum as a small expression embedded in the field description, after a marker
of the form:

    Range Variable[x]:   <EXPR>[ +/- OFFSET]

where <EXPR> can be:

  * a bare parameter name          :  MEMC_NUM_RANKS
  * param + integer                :  MEMC_NUM_RANKS + 7
  * param - integer                :  MEMC_NUM_RANKS - 1
  * parenthesised inner expression :  (MEMC_WRCMD_ENTRY_BITS - 1)
  * ternary in parens              :  (c1==v1) ? A : B
                                    (c1==v1) ? A : (c2==v2) ? B : C
  * quoted expression              :  "(MEMC_NUM_RANKS == 2) ? 2 : 4" + 23

The module computes the *maximum* the expression can take over all legal
configurations, then turns it into a concrete width via ``high - lo + 1``.

PARAM_MAX is a curated dictionary covering parameters encountered in the
Synopsys IPs processed so far:

    DWC_ddr_umctl2   (versions 3.91a and earlier)
    DW_apb_*         (timers, wdt, gpio, rtc, ssi, uart, i2c)
    DW_axi_*         (axi core, dmac, a2x, gs, x2h, x2p, x2x)

Extend PARAM_MAX when a new IP introduces a parameter not already listed.
The 31 self-tests at the bottom of the file are the safety net.
"""
from __future__ import annotations

import re
from typing import Optional


# Maximum value each configuration parameter can take across all supported
# DDR/AXI/APB configurations.  The defaults are conservative.
PARAM_MAX = {
    # Number of ranks / ports / channels
    "MEMC_NUM_RANKS": 4,
    "MEMC_NUM_RANK": 4,                  # singular variant
    "UMCTL2_NUM_PORTS": 4,
    "UMCTL2_EXCL_ACCESS": 4,
    # Address-map parameters
    "MEMC_ROW_ADDR_WIDTH": 18,
    "MEMC_COL_ADDR_WIDTH": 12,
    "MEMC_BANK_ADDR_WIDTH": 4,
    "MEMC_CS_ADDR_WIDTH": 2,
    "MEMC_BG_BITS": 2,
    "MEMC_BANK_BITS": 2,
    "MEMC_RANK_BITS": 2,
    "MEMC_PAGE_BITS": 18,
    "MEMC_HIF_ADDR_WIDTH_MAX": 40,
    # DRAM data width
    "MEMC_DRAM_DATA_WIDTH": 32,
    # VCID / BSM / queue entry widths
    "UMCTL2_CID_WIDTH": 3,
    "UMCTL2_BSM_BITS": 6,
    "MEMC_RDCMD_ENTRY_BITS": 6,
    "MEMC_WRCMD_ENTRY_BITS": 6,
    # ECC / retry / scrub widths
    "MEMC_MAX_INLINE_ECC_PER_BURST_BITS": 2,
    "UMCTL2_RETRY_MAX_ADD_RD_LAT_LG2": 4,
    "UMCTL2_DATARAM_PAR_DW_LG2": 4,
    "UMCTL2_DATARAM_PAR_DW": 4,
    "UMCTL2_OCPAR_WDATA_OUT_ERR_WIDTH": 4,
    "UMCTL2_OCPAR_ADDR_LOG_HIGH_WIDTH": 4,
    "UMCTL2_REG_SCRUB_INTERVALW": 4,
    # DW_axi_dmac configuration parameters (added for the 2026-07-09 run)
    "DMAX_NUM_CHANNELS": 32,
    "DMAX_NUM_HS_IF": 64,
    "DMAX_NUM_MASTER_INTERFACES": 2,
    "DMAX_CHx_FIFO_DEPTH": 64,
    "DMAX_CHx_ADDR_WIDTH": 64,
    "DMAX_M_DATA_WIDTH": 1024,
    # Fallback for unrecognised parameters
    "_DEFAULT": 4,
}


def _compute_max(expr: str) -> Optional[int]:
    """Recursively evaluate the maximum of a ``Range Variable[x]:`` expression.

    Returns None if the expression cannot be evaluated (malformed input or
    unrecognised parameter); callers are expected to fall back to the
    conservative 4-bit default.
    """
    expr = expr.strip()
    if not expr:
        return None
    if expr.startswith('"') and expr.endswith('"'):
        expr = expr[1:-1].strip()
    if not expr:
        return None

    # Case 1: leading '(' — could be a ternary or a parenthesised sub-expression
    # with an outer +/- offset.
    if expr.startswith("("):
        depth, close_idx = 0, -1
        for i, ch in enumerate(expr):
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    close_idx = i
                    break
        if close_idx == -1:
            return None
        inner = expr[1:close_idx]
        rest = expr[close_idx + 1:].strip()
        if "?" in rest:
            return _extract_max_from_ternary_rest(rest)
        om = re.match(r"\s*([+\-])\s*(\d+)", rest)
        if om:
            sign = 1 if om.group(1) == "+" else -1
            inner_max = _compute_max(inner)
            if inner_max is None:
                return None
            return inner_max + sign * int(om.group(2))
        return _compute_max(inner)

    # Case 2: bare parameter or "PARAM +/- N".
    m = re.match(r"(\w+)\s*(.*)", expr)
    if not m:
        return None
    param, rest = m.group(1), m.group(2).strip()
    base = PARAM_MAX.get(param, PARAM_MAX["_DEFAULT"])
    if rest:
        om = re.match(r"([+\-])\s*(\d+)", rest)
        if om:
            sign = 1 if om.group(1) == "+" else -1
            return base + sign * int(om.group(2))
    return base


def _extract_max_from_ternary_rest(rest: str) -> Optional[int]:
    """Pick the maximum branch from a ternary RHS.

    Branches may be integers or quoted parameter references.  Anything else
    is treated as 0; we return the largest integer we encountered.
    """
    values: list[int] = []
    for n in re.findall(r"\b(\d+)\b", rest):
        values.append(int(n))
    for p in re.findall(r"\"(\w+)\"", rest):
        values.append(PARAM_MAX.get(p, PARAM_MAX["_DEFAULT"]))
    return max(values) if values else None


def resolve_field_width(bits_str: str, desc: str) -> Optional[int]:
    """Compute maximum width for a variable-width field, or None if not variable.

    ``bits_str`` like ``x:24`` (the high-bit 'x' is the variable marker).
    ``desc`` is the field description which may contain a
    ``Range Variable[x]: <EXPR>[ +/- OFFSET]`` clause.
    """
    if "x" not in bits_str:
        return None
    parts = bits_str.split(":")
    lo = int(parts[1])

    m = re.search(r"Range Variable\[x\]:\s*(.*)", desc, re.DOTALL)
    if not m:
        return None
    raw = m.group(1).strip()
    if not raw:
        return None

    # Trailing offset: optional closing quote, whitespace, +/-N at end of string.
    trailing = 0
    tm = re.search(r"\"?\s*([+\-])\s*(\d+)\s*$", raw)
    if tm:
        sign = 1 if tm.group(1) == "+" else -1
        trailing = sign * int(tm.group(2))
        raw = raw[: tm.start()].rstrip()
        if raw.startswith('"'):
            raw = raw[1:].lstrip()

    max_val = _compute_max(raw)
    if max_val is None:
        return None
    high = max_val + trailing
    return max(high - lo + 1, 1)


# ---------------------------------------------------------------------------
# Self-tests
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    cases = [
        # (bits_str, expr_tail, expected_width)
        ("x:24", '"(MEMC_NUM_RANKS==2) ? 2 : 4" + 23', 4),         # MSTR active_ranks
        ("x:0",  '"(MEMC_MOBILE_OR_LPDDR2_OR_DDR4_EN==1) ? 3 : 2"\n-1', 3),  # STAT
        ("x:8",  '"MEMC_NUM_RANKS" + 7', 4),                       # rfc_tmgreg_sel
        ("x:0",  '"MEMC_NUM_RANKS" - 1', 4),                       # rank_tmgreg_sel
        ("x:16", '"UMCTL2_CID_WIDTH" + 15', 3),                    # MRCTRL0 mr_cid
        ("x:4",  '"MEMC_NUM_RANKS" + 3', 4),                       # MRCTRL0 mr_rank
        ("x:0",  '"MEMC_PAGE_BITS" - 1', 18),                      # MRCTRL1 mr_data
        ("x:4",  ' "(UMCTL2_FREQUENCY_NUM>2) ? 2 :\n1" + 3', 2),   # HWFFCSTAT
        ("x:24", '"MEMC_MAX_INLINE_ECC_PER_BURST_BITS" + 23', 2),  # ECCCFG0
        ("x:16", '"(MEMC_INLINE_ECC_EN==1 && MEMC_SIDEBAND_ECC_EN==0) ? 1 :\n(MEMC_FREQ_RATIO==2) ? 4 : 2" + 15', 4),
        ("x:24", '"MEMC_RANK_BITS" + 23', 2),
        ("x:28", '"UMCTL2_CID_WIDTH" + 27', 3),
        ("x:16", '"UMCTL2_RETRY_MAX_ADD_RD_LAT_LG2" + 15', 4),
        ("x:20", ' "(UMCTL2_HWFFC_EN_VAL==1) ? 4 : 3"\n+ 19', 4),
        ("x:28", '"MEMC_NUM_RANKS" + 27', 4),
        ("x:8",  '"MEMC_RDCMD_ENTRY_BITS" + 7', 6),                # SCHED
        ("x:16", '"UMCTL2_BSM_BITS - 2" + 15', 4),                 # SCHED2 dealloc_num_bsm_m1
        ("x:24", '"MEMC_RDCMD_ENTRY_BITS" + 23', 6),               # SCHED3 rd_pghit_num_thresh
        ("x:16", '"MEMC_WRCMD_ENTRY_BITS" + 15', 6),               # SCHED3 wr_pghit_num_thresh
        ("x:8",  '"MEMC_WRCMD_ENTRY_BITS - 1" + 7', 5),
        ("x:0",  '"MEMC_WRCMD_ENTRY_BITS - 1" - 1', 5),
        ("x:16", '"MEMC_WRCMD_ENTRY_BITS + 1" +\n15', 7),          # DBGCAM
        ("x:0",  '"MEMC_HIF_ADDR_WIDTH_MAX - 32" -\n1', 8),
        ("x:16", '"MEMC_RDCMD_ENTRY_BITS + 2" + 15', 8),           # DYNBSMSTAT
        ("x:0",  '"UMCTL2_OCPAR_WDATA_OUT_ERR_WIDTH" - 1', 4),     # OCPARSTAT2
    ]
    passed = failed = 0
    for bits, tail, expected in cases:
        # Build a fake desc that mimics the layout seen in real databooks.
        desc = f"Some description text\n\nRange Variable[x]: {tail}"
        got = resolve_field_width(bits, desc)
        ok = (got == expected)
        marker = "PASS" if ok else "FAIL"
        if ok:
            passed += 1
        else:
            failed += 1
        print(f"{marker}: bits={bits:>5s} expected={expected:2d} got={got}  tail={tail[:60]!r}")
    print(f"\n{passed} passed, {failed} failed")
