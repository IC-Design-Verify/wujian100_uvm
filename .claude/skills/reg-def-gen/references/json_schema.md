# Input JSON Schema

The reg-def-gen pipeline consumes a single JSON document whose top-level shape
is:

```jsonc
{
  "registers": [ Register, ... ],
  "errors":    [ "REG: message", ... ]      // optional diagnostics
}
```

A `Register` object:

```jsonc
{
  "section":    1,                          // integer; group/chapter number
  "subsection": 1,                          // integer; optional
  "number":     23,                         // integer; per-chapter sequence
  "name":       "MSTR",                     // UPPER_SNAKE register name (string)
  "meta": {
    "offset":      "0x00000000",            // hex string, e.g. "0x004"
    "description": "Master Register",
    "size":        "32"                     // bits; must be 32 for the default generators
  },
  "fields": [ Field, Field, ... ]
}
```

A `Field` object:

```jsonc
{
  "bits":     "31:30",                      // "HI:LO" (each is digit or 'x'), or single "K"
  "name":     "device_config",              // empty string for reserved fields
  "access":   "R/W",                        // R/W, R, W, R/RW, W/R, R/W1C, R/W1S, ...
  "desc":     "human-readable explanation\nRange Variable[x]: \"MEMC_NUM_RANKS\" - 1",
  "reserved": false                          // true iff this is a reserved row
}
```

### Bit-spec grammar

| Form          | Meaning                              | Notes                                           |
|---------------|--------------------------------------|-------------------------------------------------|
| `"31"`        | single bit 31                        | width = 1                                       |
| `"31:30"`     | bits 31 down to 30                   | width = 2                                       |
| `"x:0"`       | variable width starting at LSB      | width resolved by `resolve_field_width`         |
| `"x:24"`      | variable width field occupying high bits | high bit = 24 + width - 1                    |

If a field uses `x:HI_or_LO` the generator emits a `variable width (x:N), default=W bits`
comment in addition to the resolved struct width.

### Reserved rows

A field with `reserved: true` (or a `name` that is the empty string) is emitted as a
`/* rsvd_HI_LO */` slot in the struct.  The generator additionally inserts **gap-fill**
reserved fields for every contiguous run of bit positions that **no** documented field
covers; this ensures every struct sums to 32 bits even when the databook omits some
reserved ranges.

### Variable-width expressions

The description of an `x:N` field may contain a clause of the form:

    Range Variable[x]:   <EXPR>[ +/- OFFSET]

EXPR can be a parameter name, a parameter +/- integer, a parenthesised ternary,
or a quoted expression (see `variable_width.md` for the full grammar).  Missing
or unrecognised parameter names fall back to the conservative 4-bit default.

### Missing fields

| Missing    | Behaviour                                                                    |
|------------|-------------------------------------------------------------------------------|
| `offset`   | register is dropped from the JSON, error recorded in `errors[]`               |
| `access`   | field is dropped from the struct                                             |
| `name`     | field is treated as reserved (no `unsigned int name ... ;` line)             |
| `desc`     | field is emitted with no trailing description comment                         |

### Worked example

See `examples/DWC_ddr_umctl2/input.json` for a 2-register subset of the real
DWC DDR controller.  Both registers have variable-width fields with
`Range Variable[x]:` clauses.
