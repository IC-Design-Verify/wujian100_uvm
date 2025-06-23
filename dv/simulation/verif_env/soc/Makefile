export LIGHT_ROOT := $(shell pwd)

c_test = timer/timer_test.c

all: comp run
all_vip: comp_vip run_vip
all_vip_dpi: comp_vip run_vip_dpi

ccomp:
	./script/make_hex $(c_test)

comp:
	vcs -debug_access -full64 -sverilog -kdb \
		-l comp.log \
		-timescale=1ns/1ps \
		-ntb_opts uvm-1.2 +define+UVM_PACKER_MAX_BYTES=1500000 +define+DEMO_MAKEFILE +define+UVM_EVENT_CALLBACK_FIX \
		-f ./soc_top/filelist/tb.f -f ./soc_top/filelist/tc.f \
		-f ./soc_top/filelist/rtl.f \
		-top tb_top

comp_vip:
	vcs -debug_access -full64 -sverilog -kdb \
		-l comp.log \
		-timescale=1ns/1ps \
		+define+USE_AHB_VIP_TO_REPLACE \
		-ntb_opts uvm-1.2 +define+UVM_PACKER_MAX_BYTES=1500000 +define+DEMO_MAKEFILE +define+UVM_EVENT_CALLBACK_FIX \
		-f ./soc_top/filelist/tb.f -f ./soc_top/filelist/tc.f \
		-f ./soc_top/filelist/rtl.f \
		-top tb_top

run: ccomp
	./simv -l simv.log +UVM_TESTNAME=soc_top_for_c_case_test

run_vip:
	./simv -l simv.log +UVM_TESTNAME=soc_top_smoke_test

run_vip_dpi:
	./simv -l simv.log +UVM_TESTNAME=soc_top_smoke_test -sv_lib test_start

clean:
	rm -rf verdi* comp.log simv* vc_hdrs.h csrc out hierarchy.txt ucli* tr_db.log *xml *log *fsdb novas*
	rm -rf core.* *.c_code test.pat

clean_all: clean
	rm -rf env filelist model  reg_model  sequences  tb_top  tests  uvc soc_top common
