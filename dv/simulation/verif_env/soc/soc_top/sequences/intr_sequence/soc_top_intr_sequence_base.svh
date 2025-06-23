`ifndef SOC_TOP_INTR_SEQUENCE_BASE__SV
`define SOC_TOP_INTR_SEQUENCE_BASE__SV
class soc_top_intr_sequence extends uvm_sequence;
  `uvm_object_utils(soc_top_intr_sequence)

  soc_top_env_config env_cfg;
  uvm_reg regs[$];
  uvm_status_e status;
  uvm_hdl_data_t value;
  process self_process;

  //------------------------------------------
  // Methods
  //------------------------------------------
  // Standard UVM Methods:
  function new(string name = "soc_top_intr_sequence");
    super.new(name);
  endfunction
  virtual task pre_start();
    if(!uvm_config_db#(soc_top_env_config)::get(get_sequencer(), "*", "soc_top_env_config", env_cfg))
      `uvm_error("CFGERR", "environment config is not set to intr sequence!")
  endtask


  // User_defined Methods
  // Task that suspends main process execution
  task suspend();
    if(self_process != null && self_process.status != process::FINISHED/* && self_process.status != process::SUSPENDED*/) begin
      if(self_process.status == process::SUSPENDED) begin
        `uvm_info("PROCESS", "Other process has been executed, waiting...", UVM_LOW)
        // Wait other irq process execute completely
        wait(self_process.status != process::SUSPENDED);
        `uvm_info("PROCESS", "Other process executed completely, wait over", UVM_LOW)
      end
      // Suspending main process
      self_process.suspend();
      // Waiting for the process status to be updated
      wait(self_process.status == process::SUSPENDED);
      `uvm_info("PROCESS", "Suspending main process", UVM_LOW)
    end
  endtask: suspend

  // Task that resumes main process execution
  task resume();
    if(self_process != null && self_process.status != process::FINISHED) begin
      // resuming main process
      self_process.resume();
      // Waiting for the process status to be updated
      wait(self_process.status == process::RUNNING);
      `uvm_info("PROCESS", "Resuming main process", UVM_LOW)
    end
  endtask

endclass
`endif

