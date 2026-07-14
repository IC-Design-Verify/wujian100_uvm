# Stage 1 Output Schemas

## spec_analysis.json

```json
{
  "design_name": "string (required)",
  "spec_source": "string — path to spec document",
  "interfaces": [
    {
      "name": "string — interface name (e.g. apb_if, uart_if)",
      "type": "string — protocol type (APB/UART/SPI/AXI/custom)",
      "role": "master | slave | monitor",
      "signals": [
        {
          "name": "string",
          "direction": "input | output | inout",
          "width": "integer",
          "description": "string"
        }
      ]
    }
  ],
  "registers": [
    {
      "name": "string",
      "offset": "hex string (e.g. 0x00)",
      "width": "integer (bits)",
      "access": "RW | RO | WO | W1C",
      "reset_value": "hex string",
      "fields": [
        {
          "name": "string",
          "bits": "string (e.g. [7:0] or [3])",
          "access": "RW | RO | WO | W1C",
          "description": "string",
          "encoding_note": "string (optional) — when storage encoding differs from user semantics"
        }
      ]
    }
  ],
  "fsm_summary": [
    {
      "name": "string",
      "module": "string — RTL module name",
      "state_register": "string — RTL signal name",
      "states": ["string"],
      "reset_state": "string",
      "description": "string"
    }
  ],
  "macros": [
    {
      "name": "string (UPPER_CASE)",
      "value": "string",
      "source": "RTL | Spec | derived",
      "verification_impact": "string"
    }
  ],
  "clock_reset": {
    "clock_name": "string",
    "clock_freq_mhz": "number",
    "reset_name": "string",
    "reset_polarity": "active_low | active_high",
    "reset_type": "synchronous | asynchronous"
  },
  "error_conditions": [
    {
      "name": "string",
      "trigger": "string — trigger condition",
      "flag_register": "string — which register/bit reports this",
      "handling": "string — DUT behavior"
    }
  ],
  "mismatches": [
    {
      "location": "string",
      "description": "string",
      "severity": "high | medium | low",
      "recommendation": "string"
    }
  ]
}
```

### Minimum Viable Quality (MVQ) Criteria for spec_analysis.json

Before declaring Stage 1 output ready, verify ALL of the following:

| Field | Minimum requirement | Escalate if |
|-------|-------------------|-------------|
| `design_name` | Non-empty string | Empty or generic placeholder |
| `interfaces` | At least 1 entry with >= 1 signal | Empty array |
| `interfaces[].signals` | Each interface has >= 1 signal with valid direction and width > 0 | Any signal has width=0 or missing direction |
| `registers` | At least 1 entry (most designs have config registers) | Empty array (warn, may be valid for pure combinational designs) |
| `registers[].offset` | Unique per register (no address collision) | Duplicate offsets found |
| `registers[].fields` | Each register has >= 1 field | Any register has empty fields array |
| `clock_reset` | `clock_name` and `reset_name` are non-empty | Either is empty |
| `clock_reset.reset_polarity` | One of: `active_low`, `active_high` | Missing or invalid value |

**Per-field confidence scoring**: Each top-level section should include an optional `"confidence"` field:
```json
{
  "interfaces": [...],
  "interfaces_confidence": "high | medium | low",
  "registers": [...],
  "registers_confidence": "high | medium | low",
  ...
}
```

Confidence levels:
- `high`: extracted from clear spec text + confirmed by RTL structure
- `medium`: extracted from spec but RTL confirmation ambiguous or partial
- `low`: inferred or guessed — requires human review before downstream stages proceed

**Pipeline gate**: If any `*_confidence` field is `"low"`, the pipeline-orchestrator must pause and ask the user to review before proceeding to Stage 2.

## fsm_analysis.json

```json
{
  "design_name": "string",
  "fsm_list": [
    {
      "name": "string",
      "module": "string",
      "hierarchy_path": "string",
      "state_register": "string",
      "encoding": "binary | onehot | gray | enum",
      "states": [
        {
          "name": "string",
          "value": "string (e.g. 3'b001)",
          "description": "string",
          "source": "spec | rtl | both"
        }
      ],
      "transitions": [
        {
          "from": "string",
          "to": "string",
          "condition": "string",
          "source": "spec | rtl | both",
          "priority": "high | medium | low"
        }
      ],
      "reset_state": "string",
      "mismatches": [
        {
          "type": "missing_state | extra_state | missing_transition | extra_transition",
          "description": "string",
          "severity": "high | medium | low"
        }
      ]
    }
  ]
}
```

## testplan.json

```json
{
  "design_name": "string",
  "version": "string",
  "scenarios": [
    {
      "id": "string (e.g. FUNC_001)",
      "group": "string (e.g. Basic_TX | Error_Handling | Config | Reset)",
      "name": "string",
      "description": "string",
      "priority": "P0 | P1 | P2",
      "preconditions": ["string"],
      "steps": ["string"],
      "expected_result": "string",
      "coverage_intent": ["string — related coverpoint or metric"],
      "assertion_intent": ["string — related assertion"],
      "status": "not_covered | partial | covered",
      "covered_by": "string | null — test class name",
      "vseq": "string | null — virtual sequence name",
      "source_files": ["string — code file paths"],
      "review_note": "string — what a human reviewer should check"
    }
  ]
}
```

## escalation_report.json

Unified escalation schema used across all pipeline stages when issues require human review.

```json
{
  "stage": "string — which stage triggered escalation (e.g. spec-to-testplan)",
  "timestamp": "ISO-8601",
  "round": "integer — which fix/analysis round triggered escalation (1 for single-pass stages)",
  "unresolved_errors": [
    {
      "category": "string — error category (e.g. spec_ambiguity, rtl_mismatch, compile_error)",
      "file": "string — source file path (or null if not file-specific)",
      "line": "integer | null",
      "description": "string — human-readable description of the issue",
      "attempted_fix": "string | null — what auto-fix was tried, if any",
      "reason_escalated": "string — why auto-fix was not possible or not appropriate"
    }
  ],
  "recommendation": "string — suggested human action"
}
```

This schema is shared across all stages. Each stage's SKILL.md defines its own escalation triggers; the output format is always this schema.

## Example: Industry DVPlan-style Testplan Mapping

The `testplan.json` structure maps to industry DVPlan/vPlan columns:

| testplan.json field | DVPlan equivalent |
|--------------------|--------------------|
| `id` | Feature ID |
| `group` | Feature Group |
| `name` | Test Name |
| `description` | Description |
| `priority` | Priority |
| `coverage_intent` | Coverage Link |
| `status` | Status (auto-updated) |
| `covered_by` | Implemented By |
