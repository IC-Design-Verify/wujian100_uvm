`ifndef SOC_TOP_SMOKE_TEST__SV
`define SOC_TOP_SMOKE_TEST__SV
class soc_top_smoke_test extends soc_top_test_base;

  // UVM Factory Registration Macro
  //
	`uvm_component_utils(soc_top_smoke_test)

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
	extern function new(string name="soc_top_smoke_test", uvm_component parent);
	extern function void build_phase(uvm_phase phase);
	extern function void connect_phase(uvm_phase phase);
	extern task run_phase(uvm_phase phase);

  // User-Defined Methods:


endclass: soc_top_smoke_test

function soc_top_smoke_test::new(string name="soc_top_smoke_test", uvm_component parent);
	super.new(name, parent);
endfunction: new

function void soc_top_smoke_test::build_phase(uvm_phase phase);
	super.build_phase(phase);
  //env_cfg.has_soc_top_vseqr = 1;
	uvm_config_db #(uvm_object_wrapper)::set(this,"*m_env.soc_top_vseqr.run_phase","default_sequence", null);
  //uvm_config_db #(int)::set(this,"*m_env.soc_top_vseqr.*","item_count", 10);
endfunction: build_phase

function void soc_top_smoke_test::connect_phase(uvm_phase phase);
	super.connect_phase(phase);
  //<agent_name>_drv_exp_cb = new();
  //uvm_callbacks #(<agent_name>_driver, <agent_name>_driver_callback)::add(m_env.<agent_name>_agt.drv, <agent_name>_drv_exp_cb);
  //uvm_callbacks #(<agent_name>_driver, <agent_name>_driver_callback)::display();

  //eot_cb = new();
  //uvm_callbacks #(xxx_scoreboard, xxx_scoreboard_callback)::add(m_env.xxx_scb, eot_cb);
endfunction: connect_phase

task soc_top_smoke_test::run_phase(uvm_phase phase);
  soc_top_smoke_v_sequence soc_top_vseq;
  phase.raise_objection(this);
  soc_top_vseq = new();
  soc_top_vseq.start(m_env.soc_top_vsqr);
  phase.drop_objection(this);
endtask

class soc_top_for_c_case_test extends soc_top_test_base;

  // UVM Factory Registration Macro
  //
	`uvm_component_utils(soc_top_for_c_case_test)

  //------------------------------------------
  // TLM/Component Members
  //------------------------------------------

  //------------------------------------------
  // Data Members
  //------------------------------------------
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();


  //------------------------------------------
  // Methods
  //------------------------------------------
  // Standard UVM Methods:
	extern function new(string name="soc_top_for_c_case_test", uvm_component parent);
	extern task run_phase(uvm_phase phase);

  // User-Defined Methods:

endclass

function soc_top_for_c_case_test::new(string name="soc_top_for_c_case_test", uvm_component parent);
	super.new(name, parent);
endfunction: new

task soc_top_for_c_case_test::run_phase(uvm_phase phase);
  uvm_object_string_pool #(uvm_event#(bit[31:0])) evt_pool;
  uvm_event#(bit[31:0])                           cpu_c_finish_e;
  bit[31:0]                                       c_sim_result;
  phase.raise_objection(this);
  evt_pool = uvm_object_string_pool #(uvm_event#(bit[31:0]))::get_global_pool();
  cpu_c_finish_e = evt_pool.get("cpu_c_finish_e");
  fork
    begin
      forever begin
        cpu_c_finish_e.wait_ptrigger_data(c_sim_result);
        if(c_sim_result==32'h1001) begin
          $display("***************************************\n");
          $display("*              Test Fail              *\n");
          $display("***************************************\n");
          `uvm_error("UVM_TEST_FAILED", "C case test fail")
          $display("WUJIAN TEST END\n");
          break;
        end
        else if(c_sim_result==32'h2002) begin
          $display("***************************************\n");
          $display("*              Test Pass              *\n");
          $display("***************************************\n");
          `uvm_info("UVM_TEST_PASS", "UVM_TEST_PASS", UVM_LOW)
          $display("WUJIAN TEST END\n");
          break;
        end
        else if(c_sim_result==32'h3003) begin
          `uvm_info("UVM_TEST_SAVED", "UVM_TEST_SAVED", UVM_LOW)
          if($test$plusargs("save")) begin
            $save("SAVE");
            `uvm_info(get_type_name(), "Saving snapshot ...", UVM_NONE)
          end
          else begin
            `uvm_info(get_type_name(), "Skipping snapshot saving...", UVM_NONE)
          end
          #1;

          if($test$plusargs("save")) begin
            break;
          end
          else if($test$plusargs("restore")) begin
            break;
          end
        end
      end
    end
  join
  phase.drop_objection(this);
endtask

class soc_top_vip_run_with_c_test extends soc_top_test_base;

  // UVM Factory Registration Macro
  //
	`uvm_component_utils(soc_top_vip_run_with_c_test)

  //------------------------------------------
  // TLM/Component Members
  //------------------------------------------

  //------------------------------------------
  // Data Members
  //------------------------------------------
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();


  //------------------------------------------
  // Methods
  //------------------------------------------
  // Standard UVM Methods:
	extern function new(string name="soc_top_vip_run_with_c_test", uvm_component parent);
	extern virtual function void build_phase(uvm_phase phase);
	extern virtual function void connect_phase(uvm_phase phase);
	extern task run_phase(uvm_phase phase);

  // User-Defined Methods:


endclass: soc_top_vip_run_with_c_test

function soc_top_vip_run_with_c_test::new(string name="soc_top_vip_run_with_c_test", uvm_component parent);
	super.new(name, parent);
endfunction: new

function void soc_top_vip_run_with_c_test::build_phase(uvm_phase phase);
	super.build_phase(phase);
  //env_cfg.has_soc_top_vseqr = 1;
	uvm_config_db #(uvm_object_wrapper)::set(this,"*m_env.soc_top_vseqr.run_phase","default_sequence", null);
  //uvm_config_db #(int)::set(this,"*m_env.soc_top_vseqr.*","item_count", 10);
endfunction: build_phase

function void soc_top_vip_run_with_c_test::connect_phase(uvm_phase phase);
	super.connect_phase(phase);
  //<agent_name>_drv_exp_cb = new();
  //uvm_callbacks #(<agent_name>_driver, <agent_name>_driver_callback)::add(m_env.<agent_name>_agt.drv, <agent_name>_drv_exp_cb);
  //uvm_callbacks #(<agent_name>_driver, <agent_name>_driver_callback)::display();

  //eot_cb = new();
  //uvm_callbacks #(xxx_scoreboard, xxx_scoreboard_callback)::add(m_env.xxx_scb, eot_cb);
endfunction: connect_phase

task soc_top_vip_run_with_c_test::run_phase(uvm_phase phase);
  soc_vip_run_with_c_v_sequence soc_top_vseq;
  phase.raise_objection(this);
  soc_top_vseq = new();
  soc_top_vseq.start(m_env.soc_top_vsqr);
  phase.drop_objection(this);
endtask


`endif

