/*Example:
covergroup reg_cov;
  FUNC_A: coverpoint a {
          bins  bin1 = {0};
          bins  bin2 = {[1:2]};
          bins  bin3 = {3, 5};
          bins  bin4 = {4};
        }
  FUNC_B: coverpoint b {
          bins  bin5 = {0};
          bins  bin6 = {1};
          bins  bin7 = {2};
          bins  bin8 = {3};
        }
  ALL: cross FUNC_A, FUNC_B

endgroup: reg_cov
*/

`ifndef SOC_TOP_FUNC_COV__SV
`define SOC_TOP_FUNC_COV__SV
class soc_top_coverage extends uvm_component;

  // UVM Factory Registration Macro
  //
  `uvm_component_utils(soc_top_coverage)

  //------------------------------------------
  // TLM/Component Members
  //------------------------------------------
  //uvm_analysis_imp #(xxx_seq_item, soc_top_coverage) cov_imp;

  //------------------------------------------
  // Data Members
  //------------------------------------------
  soc_top_env_config env_cfg;
  //xxx_seq_item tr;

  //------------------------------------------
  // Coverage
  //------------------------------------------
  covergroup reg_cov;

  endgroup: reg_cov

  //------------------------------------------
  // Methods
  //------------------------------------------
  // Standard UVM Methods:
  extern function new(string name="soc_top_coverage", uvm_component parent);
  extern function void build_phase(uvm_phase phase);
  //extern function write(xxx_seq_item req);

  // User_defined Methods

endclass: soc_top_coverage

function soc_top_coverage::new(string name="soc_top_coverage", uvm_component parent);
  super.new(name, parent);
  reg_cov = new();
  //cov_imp = new("cov_imp", this);
  reg_cov.set_inst_name({get_full_name(),".reg_cov"});
  //tr = xxx_seq_item::type_id::create("tr", this);
endfunction: new

function void soc_top_coverage::build_phase(uvm_phase phase);
  super.build_phase(phase);
  if (!uvm_config_db#(soc_top_env_config)::get(this, "*", "soc_top_env_config", env_cfg))
    `uvm_error("CFGERR", "environment config is not set to func_cov!")
endfunction

//function soc_top_coverage::write(xxx_seq_item req);
//  $cast(tr, req.clone);
//  reg_cov.sample();
//endfunction: write

`endif

