// =============================================================================
// UVM Virtual Sequence Skeleton
// Replace placeholders before use:
//   <VSEQ_NAME>  - Virtual sequence class name (e.g., apbuart_smoke_vseq)
//   <VSEQ_BASE>  - Base virtual sequence to extend (e.g., apbuart_vseq_base)
//   <VSQR_TYPE>  - Virtual sequencer type (e.g., apbuart_virtual_sequencer)
//
// Two variants:
//   A) Directed  - fixed stimulus from testplan (no rand fields)
//   B) Random    - constrained-random stimulus (rand fields + constraints)
// =============================================================================

// --- Variant A: Directed (fixed stimulus) ---
class <VSEQ_NAME> extends <VSEQ_BASE>;

  `uvm_object_utils(<VSEQ_NAME>)
  `uvm_declare_p_sequencer(<VSQR_TYPE>)

  // --------------------------------------------------------------------------
  // Constructor
  // --------------------------------------------------------------------------
  function new(string name = "<VSEQ_NAME>");
    super.new(name);
  endfunction : new

  // --------------------------------------------------------------------------
  // Body - 4-step pattern
  // --------------------------------------------------------------------------
  virtual task body();

    // Step 1: Configure
    //   Write configuration registers, set up DUT operating mode.

    // Step 2: Stimulate
    //   Drive primary stimulus through agent sequencers.

    // Step 3: Wait
    //   Wait for DUT to process stimulus (clock cycles, events, flags).

    // Step 4: Check
    //   Read back status/data registers, let scoreboard verify results.

  endtask : body

endclass : <VSEQ_NAME>


// --- Variant B: Random (constrained-random stimulus) ---
// Uncomment and use for random test generation:
//
// class <VSEQ_NAME>_random extends <VSEQ_BASE>;
//
//   `uvm_object_utils(<VSEQ_NAME>_random)
//   `uvm_declare_p_sequencer(<VSQR_TYPE>)
//
//   // Randomizable fields
//   rand bit [3:0] data1;
//   rand bit [3:0] data2;
//   rand int unsigned num_transactions;
//
//   // Constraints
//   constraint c_default {
//     num_transactions inside {[10:50]};
//     data1 dist {4'h0 := 5, 4'hF := 5, [4'h1:4'hE] := 90};
//     data2 dist {4'h0 := 5, 4'hF := 5, [4'h1:4'hE] := 90};
//   }
//
//   function new(string name = "<VSEQ_NAME>_random");
//     super.new(name);
//   endfunction : new
//
//   virtual task body();
//     // Step 1: Configure
//     //   Write config registers (same as directed)
//
//     // Step 2: Stimulate (random loop)
//     for (int i = 0; i < num_transactions; i++) begin
//       assert(std::randomize(data1, data2)) else
//         `uvm_error("VSEQ", "Randomization failed")
//       // Drive data1, data2 through agent
//     end
//
//     // Step 3: Wait
//     // Step 4: Check
//   endtask : body
//
// endclass : <VSEQ_NAME>_random
