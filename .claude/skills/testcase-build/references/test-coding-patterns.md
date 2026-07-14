# Test Coding Patterns

## 1. Common Scenario Categories (Design-Independent)

| Category | Purpose | Applicable when | Source |
|----------|---------|----------------|--------|
| Basic function | Core data path verification | Always | testplan P0 scenarios |
| Config traversal | Cover all legal configuration combinations | Design has configurable registers | spec_analysis.registers (writable fields) |
| Error injection | Verify error detection and reporting | Design has error handling logic | spec_analysis.error_conditions |
| Reset | Verify reset behavior and register defaults | Always | spec_analysis.clock_reset |
| Reset during operation | Verify reset robustness mid-transaction | Design has FSM | fsm_analysis (non-idle states) |
| Back-to-back | Verify continuous operation | Design has FIFO or pipeline | spec_analysis.interfaces |
| Register read/write | Verify register access correctness | Design has registers | spec_analysis.registers |
| Boundary values | Verify extreme input values | Always | spec_analysis.registers (field widths) |
| Interrupt | Verify interrupt generation and clear | Design has interrupt output | spec_analysis (interrupt registers) |

## 2. Virtual Sequence 4-Step Pattern

Every virtual sequence body follows this pattern:

```systemverilog
task body();
  // === Step 1: Configure ===
  // Method 1: Write configuration registers through bus agent
  // Example: set data width, enable features, set parameters
  `uvm_do_on_with(config_seq, p_sequencer.bus_sqr, {
    addr == CFG_REG_ADDR;
    data == cfg_value;
    write == 1;
  })
  // Method 2: Write configuration registers By register model
  `uvm_do(register_config_seq)

  // === Step 2: Stimulate ===
  // Send main stimulus through appropriate agent
  `uvm_do_on_with(data_seq, p_sequencer.data_sqr, {
    // Constrain stimulus per scenario requirements
  })

  // === Step 3: Wait ===
  // Wait for operation to complete
  // Options: poll status register, wait for interrupt, fixed delay
  // Prefer polling over fixed delay for robustness

  // === Step 4: Check (optional) ===
  // Additional register reads to verify status
  // Main data comparison is in scoreboard
endtask
```

## 3. Test Registration Checklist

Every new test must complete all items:

```
1. Test class has `uvm_component_utils(<test_name>)
2. Test file is `included in project package
3. Test name is added to sim/testname.f
4. Corresponding vseq class has `uvm_object_utils(<vseq_name>)
5. Vseq file is `included in project package
6. run_phase has raise_objection before vseq.start
7. run_phase has drop_objection after vseq.start returns
8. build_phase calls super.build_phase(phase)
```

## 4. Naming Convention

```
Test class:  <feature>_<scenario>_test           e.g. tx_basic_test, rx_parity_error_test
Vseq class:  <feature>_<scenario>_vseq           e.g. tx_basic_vseq, rx_parity_error_vseq
Test file:   <project>_tests.sv                  or individual <feature>_<scenario>_test.sv
Vseq file:   <project>_virtual_sequences.sv      or individual files

Random variants (constrained-random stimulus):
Test class:  <feature>_<scenario>_random_test    e.g. tx_basic_random_test
Vseq class:  <feature>_<scenario>_random_vseq    e.g. tx_basic_random_vseq
```

## 5. Random Stimulus Patterns

### 5.1 When to Use Random vs Directed

| Pattern | Use Case | Example |
|---------|----------|---------|
| **Directed** | Specific testplan scenario with known vectors | `drive_single(4'hA, 4'h5)` — tests AND: 0xA & 0x5 |
| **Random** | Coverage closure, bug hunting in input space | `std::randomize(d1, d2)` — explores all bit patterns |
| **Constrained Random** | Target specific regions of input space | `d1 dist {0 := 10, '1 := 10, [1:'hE] := 80}` |

**Key insight**: Directed tests verify **known behavior**; random tests find **unknown bugs**. A bug in XOR (`in1 ^ in2 | 4'h2`) was masked by the directed vector `0xA ^ 0x5 = 0xF` (bit[1]=1, same as bug). Random vectors like `0xF ^ 0xF = 0x0` expose it (bit[1]=0, bug forces it to 1).

### 5.2 Random vseq Pattern (std::randomize in loop)

```systemverilog
class <module>_<scenario>_random_v_sequence extends <module>_base_v_sequence;
  `uvm_object_utils(<module>_<scenario>_random_v_sequence)

  rand int unsigned num_iterations;
  constraint c_iter { num_iterations inside {[20:100]}; }

  function new(string name = "<module>_<scenario>_random_v_sequence");
    super.new(name);
  endfunction

  task body();
    write_ctrl(CTRL_VALUE);
    #100ns;

    for (int i = 0; i < num_iterations; i++) begin
      bit [DATA_WIDTH-1:0] d1, d2;
      assert(std::randomize(d1, d2)) else
        `uvm_error("VSEQ", $sformatf("Randomization failed at iter %0d", i))
      drive_single(d1, d2);
      #100ns;
    end
  endtask
endclass
```

### 5.3 Random vseq Pattern (rand class fields + constraints)

```systemverilog
class <module>_<scenario>_random_v_sequence extends <module>_base_v_sequence;
  `uvm_object_utils(<module>_<scenario>_random_v_sequence)

  rand bit [3:0] data1;
  rand bit [3:0] data2;
  rand int unsigned num_transactions;

  constraint c_basic {
    num_transactions inside {[10:50]};
    // Cover edge cases: all-zeros, all-ones, single-bit
    data1 dist {4'h0 := 10, 4'hF := 10, [4'h1:4'hE] := 80};
    data2 dist {4'h0 := 10, 4'hF := 10, [4'h1:4'hE] := 80};
  }

  function new(string name = "<module>_<scenario>_random_v_sequence");
    super.new(name);
  endfunction

  task body();
    write_ctrl(2'h3);  // XOR mode
    #100ns;
    for (int i = 0; i < num_transactions; i++) begin
      assert(this.randomize()) else `uvm_error("VSEQ", "Randomize failed")
      drive_single(data1, data2);
      #100ns;
    end
  endtask
endclass
```

### 5.4 Random Test Class Pattern

```systemverilog
class <module>_<scenario>_random_test extends <module>_test_base;
  `uvm_component_utils(<module>_<scenario>_random_test)

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    uvm_config_db #(uvm_object_wrapper)::set(this,
      "*m_env.<module>_vseqr.run_phase", "default_sequence", null);
  endfunction

  task main_phase(uvm_phase phase);
    <module>_<scenario>_random_v_sequence vseq;
    phase.raise_objection(this);
    vseq = new();
    assert(vseq.randomize()) else
      `uvm_fatal("TEST", "Random vseq randomization failed")
    vseq.start(m_env.<module>_vsqr);
    phase.drop_objection(this);
  endtask
endclass
```

### 5.5 Coverage-Driven Random Constraints

When coverage data reveals specific holes, add targeted constraints:

```systemverilog
// Coverage shows bit[1] never toggles in XOR mode
constraint c_xor_bit1_target {
  // Ensure bit[1] of result toggles: (d1[1] ^ d2[1]) must be 0 and 1
  // d1[1]==0, d2[1]==0 → result[1]=0
  // d1[1]==1, d2[1]==0 → result[1]=1
  (data1[1] ^ data2[1]) dist {0 := 50, 1 := 50};
}
```

### 5.6 Random Seed Management

- Use `+ntb_random_seed=<seed>` for reproducible random tests
- In testlist.py: `def_test('test_random', '+UVM_TESTNAME=test', '+ntb_random_seed=12345')`
- Run multiple seeds for coverage closure: `ksim --regress` or `--shuffle`

## 6. Example: Industry UVM Test Log (Expected Output)

A properly working test produces UVM log output like:

```
UVM_INFO @ 0: reporter [RNTST] Running test tx_basic_test...
UVM_INFO @ 100: uvm_test_top.env.apb_agent.monitor [APB_MON] Write: addr=0x00 data=0x41
UVM_INFO @ 5200: uvm_test_top.env.uart_agent.monitor [UART_MON] Frame received: data=0x41
UVM_INFO @ 5200: uvm_test_top.env.scoreboard [SCB] Match: expected=0x41 actual=0x41
UVM_INFO @ 6000: reporter [TEST_DONE] ** UVM TEST PASSED **

--- UVM Report Summary ---
** Report counts by severity
UVM_INFO :   12
UVM_WARNING :    0
UVM_ERROR :    0
UVM_FATAL :    0
```

Key indicators of PASS:
- `UVM_ERROR : 0`
- `UVM_FATAL : 0`
- `UVM TEST PASSED` or `TEST PASSED` in log
