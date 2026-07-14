// =============================================================================
// UVM Test Class Skeleton
// Replace placeholders before use:
//   <TEST_NAME>   - Test class name (e.g., apbuart_smoke_test)
//   <BASE_TEST>   - Base test class to extend (e.g., apbuart_base_test)
//   <VSEQ_NAME>   - Virtual sequence class name (e.g., apbuart_smoke_vseq)
//   <VSQR_PATH>   - Virtual sequencer path (e.g., env.vsqr)
//
// Two variants:
//   A) Directed  - starts vseq directly (no randomize on vseq)
//   B) Random    - calls vseq.randomize() before start
// =============================================================================

// --- Variant A: Directed test (fixed stimulus) ---
class <TEST_NAME> extends <BASE_TEST>;

  `uvm_component_utils(<TEST_NAME>)

  // --------------------------------------------------------------------------
  // Constructor
  // --------------------------------------------------------------------------
  function new(string name = "<TEST_NAME>", uvm_component parent = null);
    super.new(name, parent);
  endfunction : new

  // --------------------------------------------------------------------------
  // Build Phase
  // --------------------------------------------------------------------------
  virtual function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    uvm_config_db #(uvm_object_wrapper)::set(this,
      "*m_env.<module>_vseqr.run_phase", "default_sequence", null);
  endfunction : build_phase

  // --------------------------------------------------------------------------
  // Main Phase - directed vseq
  // --------------------------------------------------------------------------
  virtual task main_phase(uvm_phase phase);
    <VSEQ_NAME> vseq;
    phase.raise_objection(this);
    vseq = <VSEQ_NAME>::type_id::create("vseq");
    vseq.start(<VSQR_PATH>);
    phase.drop_objection(this);
  endtask : main_phase

endclass : <TEST_NAME>


// --- Variant B: Random test (constrained-random stimulus) ---
// Uncomment and use for random test generation:
//
// class <TEST_NAME>_random extends <BASE_TEST>;
//
//   `uvm_component_utils(<TEST_NAME>_random)
//
//   function new(string name = "<TEST_NAME>_random", uvm_component parent = null);
//     super.new(name, parent);
//   endfunction : new
//
//   virtual function void build_phase(uvm_phase phase);
//     super.build_phase(phase);
//     uvm_config_db #(uvm_object_wrapper)::set(this,
//       "*m_env.<module>_vseqr.run_phase", "default_sequence", null);
//   endfunction : build_phase
//
//   virtual task main_phase(uvm_phase phase);
//     <VSEQ_NAME>_random vseq;
//     phase.raise_objection(this);
//     vseq = <VSEQ_NAME>_random::type_id::create("vseq");
//     assert(vseq.randomize()) else
//       `uvm_fatal(get_name(), "Random vseq randomization failed")
//     vseq.start(<VSQR_PATH>);
//     phase.drop_objection(this);
//   endtask : main_phase
//
// endclass : <TEST_NAME>_random
