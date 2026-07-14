# Testplan Template & Industry Format Reference

## Industry Testplan Structure (DVPlan/vPlan Style)

A verification testplan in IC industry typically follows this hierarchy:

```
Testplan
├── Feature Group 1 (e.g. "Basic Data Path")
│   ├── Scenario 1.1 (P0) — core function
│   ├── Scenario 1.2 (P0) — core function variant
│   └── Scenario 1.3 (P1) — edge case
├── Feature Group 2 (e.g. "Configuration")
│   ├── Scenario 2.1 (P0) — default config
│   └── Scenario 2.2 (P1) — all config combinations
├── Feature Group 3 (e.g. "Error Handling")
│   ├── Scenario 3.1 (P1) — error injection
│   └── Scenario 3.2 (P1) — error recovery
├── Feature Group 4 (e.g. "Reset")
│   ├── Scenario 4.1 (P0) — basic reset
│   └── Scenario 4.2 (P1) — reset during operation
└── Feature Group 5 (e.g. "Interrupt")
    └── Scenario 5.1 (P1) — interrupt generation and clear
```

## Standard Feature Groups (applicable to any design)

| Group | When to include | Source |
|-------|----------------|--------|
| Basic Data Path | Always | spec_analysis.interfaces |
| Configuration | Design has configurable registers | spec_analysis.registers (writable fields) |
| Error Handling | Design has error detection | spec_analysis.error_conditions |
| Reset | Always | spec_analysis.clock_reset |
| Reset During Operation | Design has FSM | fsm_analysis (non-idle states) |
| Register Access | Design has registers | spec_analysis.registers |
| Boundary Values | Design has data width parameters | spec_analysis.registers (field widths) |
| Interrupt | Design has interrupt output | spec_analysis (interrupt registers) |
| Back-to-Back | Design has FIFO or pipeline | spec_analysis.interfaces |
| Low Power | Design has power-down mode | spec_analysis (power management) |
| FSM Coverage | Design has FSM | fsm_analysis.transitions |

## Priority Assignment Rules

| Priority | Criteria | Examples |
|----------|----------|---------|
| P0 | Core data path, must work for design to function | Basic TX/RX, reset to known state |
| P1 | Important but not blocking basic function | Error handling, config variations, interrupt |
| P2 | Corner cases, stress conditions | Back-to-back at max rate, all-configs cross |

## Scenario Template

```
ID: FUNC_001
Group: Basic_TX
Name: basic_transmit_test
Priority: P0
Description: Verify basic data transmission with default configuration
Preconditions:
  - DUT is reset
  - Default configuration applied
Steps:
  1. Apply reset sequence
  2. Configure DUT with default settings (write config registers)
  3. Write data to transmit register
  4. Wait for transmission complete
  5. Verify received data matches sent data
Expected Result: Data received correctly, no errors flagged
Coverage Intent: config_space_cg (default config bin), data_path_cp
Assertion Intent: protocol_handshake_assert, data_integrity_assert
```

## Excel Export Format

When user requests Excel output, map testplan.json to these columns:

| Column A | Column B | Column C | Column D | Column E | Column F | Column G | Column H |
|----------|----------|----------|----------|----------|----------|----------|----------|
| ID | Group | Name | Priority | Description | Expected Result | Coverage Link | Status |
| FUNC_001 | Basic_TX | basic_transmit_test | P0 | Verify basic... | Data received... | config_space_cg | not_covered |
