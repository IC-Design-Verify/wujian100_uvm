---
name: testcase-build
description: Use when the user needs to generate UVM test cases and virtual sequences from a testplan. Produces test classes, virtual sequences, and updates testname.f. Each test follows the standard factory-registered pattern with raise/drop objection. Code generation only -compilation and simulation verification is Stage 5.
license: PolyForm-Noncommercial-1.0.0
metadata:
  author: 中科麒芯
  organization: 中科麒芯智能技术（南京）有限公司
  homepage: https://www.ickylin.com/
  contact: info@ickylin.com
---

# Test Case Build

Generate UVM test classes and virtual sequences from testplan, register into regression.

## Boundaries

- **Spec-driven test generation**: all test scenarios derived from Spec behavior, NOT from observing RTL
- Only creates/modifies test and sequence files -**never modifies env components** (agent, scoreboard, driver, monitor)
- Batch generation: P0 first, wait for user/Stage 5 confirmation before P1
- Does not invoke EDA tools -compilation is Stage 5's job
- testplan.json updates are additive (status field), never removes scenarios
- Mustn't modify Reference Model and Scoreboard to align with DUT Verilog Code. The `spec_analysis.json` is the sole reference for writing the reference model.

## Input

| Item | Required | Source |
|------|----------|--------|
| testplan.json | Yes | Stage 1 output or user-provided equivalent |
| UVM environment code | Yes | Stage 2 output (need base_test, vseq_base, pkg) |
| spec_analysis.json | No | Stage 1 output (for register addresses in sequences) |

**Standalone mode**: user provides testplan (JSON or verbal description of scenarios) + existing UVM base_test class.

### Input Validation

Before starting, verify:
- `testplan.json` exists and has `scenarios[]` array with at least one entry
- Each scenario has required fields: `id`, `group`, `name`, `priority`, `steps`, `expected_result`
- At least one scenario has `priority: "P0"` (core data path must be tested)
- UVM test_base class exists in `dv/simulation/verif_env/<parent>/<module>/tests/uvm_test/` (contains class extending `uvm_test`)
- UVM v_sequence_base class exists in `dv/simulation/verif_env/<parent>/<module>/sequences/` (contains class extending `uvm_sequence`)
- If `spec_analysis.json` is used for register addresses: verify `registers[]` array is non-empty and each register has `offset` field
- If any missing, report to user with specific missing item and which upstream stage should be re-run

## Workflow

### Step 0: Standalone Entry Check

If upstream artifacts are absent:
- `testplan.json` missing -> prompt the user for:
  - Test scenarios (feature name, priority, stimulus description, expected behavior)
  - Or provide a spec/testplan document for inline extraction
- UVM test_base / v_sequence_base missing -> prompt the user for:
  - Path to existing UVM environment code
  - Or base test class name and virtual sequencer name to extend from
- `spec_analysis.json` missing -> proceed without register address lookup (user must provide register offsets in scenario descriptions if needed)

### Commands

```bash
# Input: docs/testplan.json + existing UVM env in dv/simulation/verif_env/<parent>/<module> and dv/simulation/verif_env/<parent>/common
# Output: test files in dv/simulation/verif_env/<parent>/<module> and dv/simulation/verif_env/<parent>/common, updated filelist in dv/simulation/verif_env/<parent>/<module>/filelist if needed
```

### Step 1: Extract Scenario List from Testplan

Read `testplan.json.scenarios[]`, sort by priority: P0 first, then P1, then P2.

### Step 2: Generate Test Class per Scenario

Each scenario maps to:
- 1 test class extending `<project>_base_test`
- 1 virtual sequence extending `<project>_vseq_base`

**Test generation strategy** (directed vs random):

| Scenario Type | Strategy | Rationale |
|---------------|----------|-----------|
| Basic function (P0) | **Directed** + **Random** | Directed covers known corner cases; random explores untested input space |
| Config traversal (P1) | **Directed** | Deterministic register read/write verification |
| Reset (P2) | **Directed** | Deterministic reset sequence |
| Boundary values | **Directed** + **Random** | Directed hits known boundaries; random fills gaps |
| Coverage holes | **Random** | Constrained-random fills coverage gaps after directed tests |

**When to generate random tests**: For each P0 data-path scenario, generate TWO tests:
1. **Directed test** (`<module>_<scenario>_test`): Fixed stimulus vectors from testplan steps
2. **Random test** (`<module>_<scenario>_random_test`): Constrained-random stimulus, multiple iterations

Random tests use `std::randomize()` or UVM sequence `randomize()` with constraints to generate stimulus. This catches bugs masked by specific directed vectors (e.g., XOR bit[1] bug only visible with certain input combinations).

Test class structure:

```systemverilog
`ifndef <MODULE>_<SCENARIO>_TEST__SV
`define <MODULE>_<SCENARIO>_TEST__SV
class <module>_<scenario>_test extends <module>_test_base;

  // UVM Factory Registration Macro
  //
	`uvm_component_utils(<module>_<scenario>_test)

  //------------------------------------------
  // TLM/Component Members
  //------------------------------------------

  //------------------------------------------
  // Data Members
  //------------------------------------------

  //<agent_name>_driver_example_callback <agent_name>_drv_exp_cb;

  //use for wait scoreboard compare completely
  //sb_eot_call_back eot_cb; 


  //------------------------------------------
  // Methods
  //------------------------------------------
  // Standard UVM Methods:
	extern function new(string name="<module>_<scenario>_test", uvm_component parent);
	extern function void build_phase(uvm_phase phase);
	extern function void connect_phase(uvm_phase phase);
	extern task main_phase(uvm_phase phase);

  // User-Defined Methods:


endclass: <module>_<scenario>_test

function <module>_<scenario>_test::new(string name="<module>_<scenario>_test", uvm_component parent);
	super.new(name, parent);
endfunction: new

function void <module>_<scenario>_test::build_phase(uvm_phase phase);
	super.build_phase(phase);
  //env_cfg.has_<module>_vseqr = 1;
	uvm_config_db #(uvm_object_wrapper)::set(this,"*m_env.<module>_vseqr.run_phase","default_sequence", null);
  //uvm_config_db #(int)::set(this,"*m_env.<module>_vseqr.*","item_count", 10);
endfunction: build_phase

function void <module>_<scenario>_test::connect_phase(uvm_phase phase);
	super.connect_phase(phase);
  //<agent_name>_drv_exp_cb = new();
  //uvm_callbacks #(<agent_name>_driver, <agent_name>_driver_callback)::add(m_env.<agent_name>_agt.drv, <agent_name>_drv_exp_cb);
  //uvm_callbacks #(<agent_name>_driver, <agent_name>_driver_callback)::display();

  //eot_cb = new();
  //uvm_callbacks #(xxx_scoreboard, xxx_scoreboard_callback)::add(m_env.xxx_scb, eot_cb);
endfunction: connect_phase

task <module>_<scenario>_test::main_phase(uvm_phase phase);
  <module>_<scenario>_v_sequence {module}_vseq;
  phase.raise_objection(this);
  {module}_vseq = new();
  {module}_vseq.start(m_env.{module}_vsqr);
  phase.drop_objection(this);
endtask
`endif
```

### Step 3: Generate Virtual Sequence per Scenario

vseq body follows the universal 4-step pattern:

```systemverilog
`ifndef <MODULE>_<SCENARIO>_V_SEQUENCE__SV
`define <MODULE>_<SCENARIO>_V_SEQUENCE__SV
class <module>_<scenario>_v_sequence extends {module}_v_sequence_base;
  `uvm_object_utils(<module>_<scenario>_v_sequence)
  //`uvm_declare_p_sequencer(${data.name}_vsequencer)

  function new(string name = "<module>_<scenario>_v_sequence");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    // Step 1: Configure DUT (write config registers via bus agent, or use register model)
    // Step 2: Send stimulus (drive data through agent)
    // Step 3: Wait for completion (poll status or wait event)
    // Step 4: Optional additional checks
  endtask
endclass: <module>_<scenario>_v_sequence
`endif
```

### Step 3b: Generate Randomized Virtual Sequence (for P0 data-path scenarios)

For each P0 data-path scenario, generate an additional randomized vseq that uses constrained-random stimulus. This vseq extends the same base as the directed vseq but uses randomization instead of hardcoded values.

**Randomized vseq template**:

```systemverilog
`ifndef <MODULE>_<SCENARIO>_RANDOM_V_SEQUENCE__SV
`define <MODULE>_<SCENARIO>_RANDOM_V_SEQUENCE__SV
class <module>_<scenario>_random_v_sequence extends <module>_base_v_sequence;
  `uvm_object_utils(<module>_<scenario>_random_v_sequence)

  // Randomizable fields
  rand bit [DATA_WIDTH-1:0] data1;
  rand bit [DATA_WIDTH-1:0] data2;
  rand int unsigned         num_transactions;

  // Constraints
  constraint c_num_trans {
    num_transactions inside {[10:50]};
  }

  // Optional: constrain data to interesting regions
  constraint c_data_range {
    // Add scenario-specific constraints here
    // e.g., cover all bit transitions:
    data1 dist {0 := 5, {DATA_WIDTH{1'b1}} := 5, [1:2**DATA_WIDTH-2] := 90};
  }

  function new(string name = "<module>_<scenario>_random_v_sequence");
    super.new(name);
    `uvm_info("TRACE", $sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    `uvm_info("VSEQ", $sformatf("Starting random %0d transactions", num_transactions), UVM_LOW)

    // Step 1: Configure DUT
    write_ctrl(CTRL_VALUE);  // Use scenario-specific config
    #100ns;

    // Step 2: Drive random stimulus
    for (int i = 0; i < num_transactions; i++) begin
      bit [DATA_WIDTH-1:0] d1, d2;
      // Re-randomize per iteration for diverse stimulus
      assert(std::randomize(d1, d2) with {
        // Add per-iteration constraints if needed
      }) else `uvm_error("VSEQ", "Randomization failed")
      drive_single(d1, d2);
      #100ns;
    end

    `uvm_info("VSEQ", "Random test completed", UVM_LOW)
  endtask
endclass: <module>_<scenario>_random_v_sequence
`endif
```

**Random test class template** (paired with random vseq):

```systemverilog
`ifndef <MODULE>_<SCENARIO>_RANDOM_TEST__SV
`define <MODULE>_<SCENARIO>_RANDOM_TEST__SV
class <module>_<scenario>_random_test extends <module>_test_base;
  `uvm_component_utils(<module>_<scenario>_random_test)

  function new(string name="<module>_<scenario>_random_test", uvm_component parent);
    super.new(name, parent);
  endfunction: new

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    uvm_config_db #(uvm_object_wrapper)::set(this,
      "*m_env.<module>_vseqr.run_phase","default_sequence", null);
  endfunction: build_phase

  task main_phase(uvm_phase phase);
    <module>_<scenario>_random_v_sequence vseq;
    phase.raise_objection(this);
    vseq = new();
    assert(vseq.randomize()) else
      `uvm_fatal("TEST", "Random vseq randomization failed")
    vseq.start(m_env.<module>_vsqr);
    phase.drop_objection(this);
  endtask
endclass: <module>_<scenario>_random_test
`endif
```

**Key differences from directed tests**:
1. Random vseq has `rand` fields with constraints
2. `body()` uses `std::randomize()` in a loop for multiple iterations
3. Test class calls `vseq.randomize()` before `vseq.start()`
4. Random test name suffix: `_random_test` / `_random_v_sequence`

### Step 3: Generate Register Sequence per Virtual Sequence

Only when Register model used in UVM Testbench and register defined in Register Model, this step will be executed.

```systemverilog
`ifndef <SCENARIO>_REG_SEQ__SV
`define <SCENARIO>_REG_SEQ__SV
class <scenario>_reg_sequence extends <scenario>_reg_sequence_base;
  // UVM Factory Registration Macro
  //
  `uvm_object_utils(<scenario>_reg_sequence)
  //`uvm_declare_p_sequencer(xxx_sequencer)

  //------------------------------------------
  // Data Members
  //------------------------------------------

  //------------------------------------------
  // Component Members
  //------------------------------------------

  //------------------------------------------
  // Methods
  //------------------------------------------

  // Standard UVM Methods:  
  extern function new(string name = "<scenario>_reg_sequence");


  task body();
    uvm_status_e status;
    uvm_reg_data_t data;
    // For rw property register, use set+update to execute write operation
    <register_model_inst_name>.<register1>.<field1>.set(<value1>);
    <register_model_inst_name>.<register1>.<field2>.set(<value2>);
    <register_model_inst_name>.<register2>.<field3>.set(<value3>);
    <register_model_inst_name>.update(status, UVM_FRONTDOOR);

    // For wc/w1c/w0c/ws/w1s/w0s property register, use write to execute write operation
    <register_model_inst_name>.<register3>.write(status, <value4>, UVM_FRONTDOOR);

    // For get register value requirment, use read to  to execute read operation
    <register_model_inst_name>.<register3>.write(status, data, UVM_FRONTDOOR);

endclass: <scenario>_reg_sequence

function <scenario>_reg_sequence::new(string name = "<scenario>_reg_sequence");
  super.new(name);
endfunction

`endif
```

### Step 6: Update Testplan with AI Coverage Annotation

For each generated test, update `testplan.json` scenario entry:

| Field | Update to |
|-------|----------|
| `status` | `"covered"` |
| `covered_by` | `"<test_class_name>"` -exact class name |
| `vseq` | `"<vseq_class_name>"` -exact vseq name |
| `source_files` | `["dv/simulation/verif_env/<parent>/<module>/tests/uvm_test/<test_file>.svh", "dv/simulation/verif_env/<parent>/<module>/sequences/<vseq_file>.svh"]` -full file paths |
| `review_note` | Brief AI description: what this test verifies, key stimulus, expected behavior |

These annotations enable human reviewers to:
- See which testplan scenarios AI has covered vs not covered
- Locate the exact source code for waveform review
- Understand AI's verification intent per scenario

### Step 7: Register to Package and Regression

1. Add `include` lines to package file:
   - **Standard structure** (generated by `light_gui`): insert after `v_sequence_base`/`test_base` include, before `endpackage`
   - **Non-standard structure**: read the package file, find the last `include` line, insert after it but before `endpackage`. If no `include` lines exist, insert before `endpackage`. If `endpackage` not found, report error -package file may be malformed.
   - **Dependency rule**: test vseq files must be included BEFORE test class files (vseqs are used inside test classes)
   - **Never duplicate**: check if the include already exists before adding
2. Add test names to `sim/testname.f` (one per line, no duplicates, no comments)

### Step 8: Batch by Priority

1. Generate all P0 tests first (directed + random for each data-path scenario)
2. Output P0 test list -user takes this to Stage 4 for compile+sim verification
3. After P0 passes, generate P1 tests (directed only)
4. After P1 passes, generate P2 tests (directed only)

This prevents generating 30 broken tests at once.

**P0 batch includes both directed and random tests**:
- Directed tests verify specific testplan scenarios with fixed vectors
- Random tests explore broader input space with constrained-random stimulus
- Random tests are named `<module>_<scenario>_random_test` and run with `--code_cov` for coverage closure

## Output

| Item | Path |
|------|------|
| Test classes | `uvm_tb/<project>_tests.sv` (or individual files) |
| Virtual sequences | `uvm_tb/<project>_test_vseqs.sv` (independent file -do NOT append to Stage 2's `*_virtual_sequences.sv`) |
| Test name list | `sim/testname.f` (updated) |
| Testplan update | `docs/testplan.json` -`status` and `covered_by` fields updated |
| Batch status | Printed to console | "P0 batch complete: N tests generated, ready for Stage 5" |

## Self-Correction

| Check | Condition | Auto-fix |
|-------|-----------|----------|
| Factory registration | Every test has `uvm_component_utils` | Add macro |
| Factory registration | Every vseq has `uvm_object_utils` | Add macro |
| Objection pairing | Every `raise_objection` has matching `drop_objection` | Add missing drop |
| Super call | Every overridden phase method calls `super.<phase>(phase)` | Add call |
| Testname.f sync | Every test class name appears in testname.f | Add missing entries |
| Package include | Every new file is included in package | Add include line |
| No duplicate tests | Test name not already in testname.f | Skip generation |
| Random vseq has rand fields | Random vseq contains `rand` variables | Add rand fields with constraints |
| Random test calls randomize() | Random test's main_phase calls `vseq.randomize()` before start | Add randomize call |
| Random vseq has constraints | Random vseq has `constraint` blocks | Add default constraints (num_transactions, data range) |
| Random test in testlist | Random test name appears in testlist .py | Add def_test entry |

**Maximum rounds**: 3 rounds of self-correction per priority batch. This limit is non-negotiable.

**Escalation trigger**: Produce `escalation_report.json` when ANY of:
- Factory registration cannot be added automatically (class structure incompatible with UVM macros)
- Objection raise/drop cannot be paired (complex control flow with multiple return paths)
- Base test class or vseq_base class is missing required methods

Format follows the unified escalation schema (defined in spec-to-testplan/references/output-schema.md#escalation_reportjson). Human action: review test class structure, fix inheritance issues, re-run.

## Standards Reference

- IEEE 1800.2-2020: uvm_test, uvm_sequence, factory registration, objection mechanism
- UVM Cookbook: test class patterns, virtual sequence coordination

## References

- [test-coding-patterns.md](references/test-coding-patterns.md) -Read during Steps 2-3 for test class skeleton, vseq 4-step pattern, registration checklist, and naming convention
- [output-schema.md](references/output-schema.md) -Read after generation to verify testname.f format and testplan.json field updates
- [test_class_skeleton.sv](assets/skeletons/test_class_skeleton.sv) -UVM test class skeleton with placeholder markers
- [vseq_skeleton.sv](assets/skeletons/vseq_skeleton.sv) -Virtual sequence skeleton with 4-step pattern
- [testname_template.f](assets/templates/testname_template.f) -testname.f format template
