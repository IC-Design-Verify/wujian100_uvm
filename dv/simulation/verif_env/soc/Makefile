export LIGHT_ROOT := $(shell pwd)

all: comp run

comp:
	vcs -debug_access -full64 -sverilog -kdb \
		-l comp.log \
		-timescale=1ns/1ps \
		-ntb_opts uvm-1.2 +define+UVM_PACKER_MAX_BYTES=1500000 +define+DEMO_MAKEFILE +define+UVM_EVENT_CALLBACK_FIX \
		-f ./soc_top/filelist/tb.f -f ./soc_top/filelist/tc.f \
		-f ./soc_top/filelist/rtl.f \
		-top tb_top

run:
	./simv -l simv.log +UVM_TESTNAME=soc_top_smoke_test

clean:
	rm -rf verdi* comp.log simv* vc_hdrs.h csrc out hierarchy.txt ucli* tr_db.log *xml *log *fsdb novas*
	rm -rf core.*

clean_all: clean
	rm -rf env filelist model  reg_model  sequences  tb_top  tests  uvc soc_top common
