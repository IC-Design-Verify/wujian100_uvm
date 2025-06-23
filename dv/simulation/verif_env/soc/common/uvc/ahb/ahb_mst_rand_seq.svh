class ahb_mst_rand_seq extends ahb_base_seq;
  `uvm_object_utils(ahb_mst_rand_seq)
  
  rand int item_count;

  
  function new(string name = "ahb_mst_rand_seq");
    super.new(name);
  endfunction: new
  
  virtual task body();
    ahb_seq_item rsp;
    
    `info($sformatf("item_count = %0d", item_count))
    fork
      // send requests process
      for(int ii = 0; ii < item_count; ii++) begin
        req = ahb_seq_item::type_id::create();
        start_item(req);
        `check_rand(req.randomize())
        finish_item(req);
        `info_high($sformatf("#%0d req - %s", ii, req.cmd2string()))
      end // for
      
      // get responses process
      for(int ii = 0; ii < item_count; ii++) begin
        get_response(rsp);
        `info_high($sformatf("#%0d rsp - %s", ii, rsp.rsp2string()))
      end // for
    join

    #100ns;
  endtask // body

  constraint item_count_c {
    item_count inside { [10 : 20] };
  }
endclass // ahb_mst_rand_seq

class ahb_master_base_sequence extends ahb_base_seq;
  `uvm_object_utils(ahb_master_base_sequence)
  function new(string name="ahb_master_base_sequence");
    super.new(name);
  endfunction

  virtual task mem_write32_(input bit[39:0] addr, input bit[31:0] data, input bit[15:0] wstrb = 16'hf);
    ahb_seq_item rsp;
    req = ahb_seq_item::type_id::create();
    start_item(req);
    `check_rand(req.randomize() with {
        req.kind        == ahb_pkg::WRITE;
        req.length      == 0;
    })
    req.addr        = addr[31:0];
    req.data[0]     = data;
    finish_item(req);
    `info_high($sformatf("# req - %s", req.cmd2string()))

    /** Send the write transaction */
    get_response(rsp);
    `info_high($sformatf("# rsp - %s", rsp.rsp2string()))

    `uvm_info("body", "AHB WRITE transaction completed", UVM_LOW);

  endtask


  virtual task mem_read32_(input bit[39:0] addr, output bit[31:0] data);
    ahb_seq_item rsp;
    req = ahb_seq_item::type_id::create();
    start_item(req);
    `check_rand(req.randomize() with {
        req.kind        == ahb_pkg::READ;
        req.length      == 0;
    })
    req.addr        = addr;
    finish_item(req);
    `info_high($sformatf("# req - %s", req.cmd2string()))

    /** Send the write transaction */
    get_response(rsp);
    `info_high($sformatf("# rsp - %s", rsp.rsp2string()))

    data = rsp.data[0][31:0];

    `uvm_info("body", $sformatf("AHB READ transaction completed, %h, %p", data, rsp.data), UVM_LOW);

  endtask

endclass
