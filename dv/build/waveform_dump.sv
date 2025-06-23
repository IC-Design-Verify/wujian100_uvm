`ifndef TB_TOP_NAME
  `define TB_TOP_NAME tb_top
`endif

`define _QUOTE_(STR) `"STR`"

module waveform_dump;

// +fsdb+all[=on|off]
// +fsdb+delta
// +fsdb+dumpon+time[+htime]
// +fsdb+dumpoff+time[+htime]

// +fsdb+mda[=on|off]
// +fsdb+packedmda[=on|off]
// +fsdb+struct[=on|off]
// +fsdb+strength[=on|off]
// +fsdb+functions  // need -debug_access+dmptf
// +fsdb+flush_period=seconds
// +fsdb+power
// +fsdb+io_only
// +fsdb+sva_status

// +fsdb+virtual_file
// +fsdbfile+filename
// +fsdb+dump_limit=size

`ifdef DUMP_FSDB
  initial begin
    if (!$test$plusargs("no_dump")) begin
      int level = 0;
      string inst_name = `_QUOTE_(`TB_TOP_NAME);

      void'($value$plusargs("dump_level=%d", level));
      void'($value$plusargs("dump_scope=%s", inst_name));
      $fsdbDumpSVA("level=", level, "instance=", inst_name);
      $fsdbDumpvars("level=", level, "instance=", inst_name);
    end
  end
`endif

endmodule
