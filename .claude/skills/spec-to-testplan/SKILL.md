---
name: spec-to-testplan
description: "Use when the user needs to analyze a design specification (PDF/document) and RTL source to extract verification-relevant information and produce a structured testplan. Covers： spec analysis, FSM extraction, register encoding analysis, and testplan generation. This is the first stage of a UVM verification pipeline -no EDA tools required."
license: "PolyForm-Noncommercial-1.0.0"
metadata:
  author: "中科麒芯"
  organization: "中科麒芯智能技术（南京）有限公司"
  homepage: "https://www.ickylin.com/"
  contact: "info@ickylin.com"
---

# Spec to Testplan

Read design specification and RTL, extract verification structures, produce testplan.

## Boundaries

- **Read-only**: this stage does not create or modify any code files
- **Spec authority**: when spec and RTL contradict, spec is the reference, RTL is flagged as mismatch
- **No EDA tools**: pure analysis, no compilation or simulation
- **No hallucination**: uncertain inferences marked `confidence: "low"`, not stated as fact
- **Anti-cheating**: test scenarios must be derived from Spec behavior, NOT from observing RTL implementation
- **Only Read DUT top module Port**: Never analyze the internal logic of the DUT

## Input

| Item | Required | Source |
|------|----------|--------|
| Design specification | Yes | User provides path. Supported formats: **PDF**, **Markdown** (.md), **Word** (.docx), **plain text** (.txt) |
| RTL top-level file (.v/.sv) | Yes | User provides path |
| RTL defines file | No | User provides path |
| input_config.json | No | Project root (if pipeline mode) |

**Standalone mode**: user provides spec + RTL paths directly.
**Pipeline mode**: reads paths from `input_config.json`.

### Input Validation

Before starting, verify:
- Spec document path exists and is readable (PDF or text)
- RTL top-level file exists and has valid SystemVerilog syntax (at minimum: `module` keyword present)
- If either is missing, report to user and stop

## Workflow

### Commands

```bash
# Pipeline mode: reads paths from input_config.json automatically
# Standalone mode: provide files directly
# Example: "Analyze spec at docs/spec.pdf with RTL at design/top.sv"

# RTL structure extraction (run BEFORE agent analysis):
python3 scripts/parse_rtl_structure.py --dir <rtl_directory>
# Output: JSON with modules, ports, parameters, defines, FSM candidates, instantiations
```

### Step 0: Input Readiness Check

Before proceeding, confirm the user has provided:
- **Design specification** -the golden reference for verification. Supported formats: **PDF**, **Markdown** (.md), **Word** (.docx), **plain text** (.txt)
- **RTL top-level file** (.v/.sv) -the DUT to be verified
- **RTL defines file** (optional) -macro definitions if used

If any required input is missing, explicitly tell the user what is still needed and what formats are accepted:
- Spec missing ->"Please provide the design specification. Supported formats: PDF, Markdown (.md), Word (.docx), or plain text (.txt)"
- RTL missing ->"Please provide the RTL top-level file path (.v or .sv)"

Do not proceed until both spec and RTL are available.

### Step 0b: RTL Structure Extraction (automated)

Before agent analysis, run `scripts/parse_rtl_structure.py` on the RTL directory to get structured data:
- Module port lists (name, direction, width)
- Parameter/localparam values
- `define macros
- FSM state register candidates (heuristic: case statement variables with 2+ labeled states)
- Module instantiation hierarchy

This gives the agent concrete data to work with, rather than relying purely on reading RTL source code. The agent then cross-references this structured data with the spec document.

**Note**: `width=-1` in the output means the port width contains a macro expression (e.g., `` `DATA_WIDTH-1:0 ``). The agent must resolve these using the extracted `defines` list.

#### Commands

```bash
python3 .claude/skills/spec-to-testplan/scripts/parse_rtl_structure.py --dir <rtl_directory>
```

### Step 1: Spec Analysis (Spec-driven, RTL as structural evidence)

Read the spec document **first**. RTL is the DUT, not the golden reference.

Extract:
1. **Interfaces** -signal names, directions, widths, protocol type
2. **Registers** -name, offset, width, access type (RW/RO/WO/W1C), reset value, field encoding
3. **Clock/Reset** -clock name, frequency, reset polarity and type
4. **Error conditions** -trigger, flag register, DUT handling behavior
5. **Macros/Parameters** -design-wide constants with verification impact

Rules:
1. Any mismatch between DUT and spec, subject to spec. This description should write to `docs/spec_analysis.json`

Then read RTL to confirm structure. Flag any spec-RTL mismatches in the output with severity. - **spec is golden**: Reference model must match DUT behavior descript in spec, but not the DUT itself.

### Step 2: FSM Extraction (Dual-source: Spec + RTL)

1. Extract FSM from RTL: identify `always` blocks with state registers, enumerate states and transitions
2. Extract FSM behavior from Spec: state descriptions, transition conditions, expected behavior
3. Cross-compare:
   - State in Spec but not in RTL ->`missing_state` (severity: high)
   - State in RTL but not in Spec ->`extra_state` (severity: medium, may be implementation detail)
   - Transition in Spec but not in RTL ->`missing_transition` (severity: high)
4. Record encoding type: binary / onehot / gray / enum

**Critical rule**: When Spec and RTL show different FSM transition paths:
- Spec transitions are the **golden reference** for test generation
- RTL-only transitions (not in Spec) are flagged as `extra_transition` for human review
- Tests must drive DUT through **Spec-defined state sequences**, not RTL-implemented ones
- If RTL lacks a Spec-required transition, this is a design bug -flag as `missing_transition` (severity: high)

### Step 3: Register Encoding Analysis

1. Compare spec-defined register field values with RTL implementation
2. Detect encoding mismatches:
   - Field width inconsistency (spec says 2-bit, RTL uses 3-bit)
   - Reset value mismatch
   - Access type mismatch (spec says RO, RTL allows write)
3. Flag mismatches with severity and recommendation

### Step 4: Testplan Generation

From the extracted structures, generate verification scenarios:

1. **Group by feature**: Basic function, Configuration, Error handling, Reset, Boundary, Interrupt
2. **Assign priority**: P0 (core data path), P1 (error/config), P2 (corner case)
3. **Link coverage intent**: each scenario maps to expected coverpoints or assertions
4. **Completeness check**:
   - Every register has at least 1 read/write scenario
   - Every FSM state is reached by at least 1 scenario
   - Every error condition has a dedicated scenario
   - Gaps are flagged in the output

## Output

Three JSON files with fixed schemas. See [output-schema.md](references/output-schema.md) for full definitions.

| File | Content |
|------|---------|
| `docs/spec_analysis.json` | Interfaces, registers, clock/reset, errors, macros, mismatches |
| `docs/fsm_analysis.json` | FSM list with states, transitions, encoding, mismatches |
| `docs/testplan.json` | Scenario list with group, priority, coverage intent, status |

## Self-Correction

| Check | Trigger | Action |
|-------|---------|--------|
| Spec-RTL signal name mismatch | Signal in spec not found in RTL port list | Flag in `mismatches[]`, severity: high |
| Register address collision | Two registers share same offset | Flag, severity: high |
| FSM state coverage gap | testplan has no scenario reaching a state | Auto-add a gap scenario with `status: "not_covered"` |
| Register coverage gap | A register has no read/write scenario | Auto-add a gap scenario |
| Empty error_conditions | Spec mentions errors but extraction found none | Warning in output, suggest manual review |

**Output quality gate**: Before finalizing, verify spec_analysis.json against the Minimum Viable Quality (MVQ) criteria defined in [output-schema.md](references/output-schema.md). All MVQ checks must pass. Include per-section confidence scores (`high`/`medium`/`low`) in the output.

**Maximum rounds**: Single-pass analysis, no iterative loop.

**Escalation trigger**: Generate `escalation_report.json` when ANY of:
- Spec document is unreadable or ambiguous (>3 registers with uncertain field definitions)
- RTL top module has no recognizable interface ports
- Spec-RTL mismatch count exceeds 5 items
- Any register has conflicting access definitions between spec and RTL

Escalation format follows the unified escalation schema defined in this skill's [output-schema.md](references/output-schema.md#escalation_reportjson). Human action: clarify spec ambiguities, confirm register definitions, re-run.

## Standards Reference

- IEEE 1685 (IP-XACT): register description field model reference
- HLVS methodology: testplan hierarchical structure (feature ->scenario ->step)
- Verification Academy: testplan completeness criteria

## References

- [output-schema.md](references/output-schema.md) -Read when producing output files, to verify JSON structure matches expected schema
- [testplan-template.md](references/testplan-template.md) -Read when generating testplan, for industry DVPlan-style format and Excel column mapping
- [spec_analysis_template.json](assets/templates/spec_analysis_template.json) -spec analysis output template
- [fsm_analysis_template.json](assets/templates/fsm_analysis_template.json) -FSM analysis output template
- [testplan_template.json](assets/templates/testplan_template.json) -testplan output template
- [spec_analysis_example.json](assets/examples/spec_analysis_example.json) -example spec analysis output (SPI master)
