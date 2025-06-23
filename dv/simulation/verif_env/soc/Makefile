export LIGHT_ROOT := $(shell pwd)

#addr_map/map_test.c
#gpio/gpio_test.c
#had_soc/e902_had_test.c
#pwm/pwm_test.c
#usi_i2c/usi_i2c_test.c
#usi_spi/usi_spi_test.c
#usi_uart/usi_uart_test.c
#dma/dma_test.c
#rtc/rtc_test.c
#timer/timer_test.c
#wdt/wdt_test.c
C_TEST = timer/timer_test.c

ifeq ($(findstring dma_test, ${C_TEST}), dma_test)
  DEF =
else ifeq ($(findstring gpio_test, ${C_TEST}), gpio_test)
  DEF = USE_APB1+USE_APB1_GPIO
else ifeq ($(findstring usi_spi_test, ${C_TEST}), usi_spi_test)
  DEF = USE_APB0+USE_APB0_USI+USI_SPI
else ifeq ($(findstring usi_i2c_test, ${C_TEST}), usi_i2c_test)
  DEF = USE_APB0+USE_APB0_USI+USI_I2C
else ifeq ($(findstring usi_uart_test, ${C_TEST}), usi_uart_test)
  DEF = USE_APB0+USE_APB0_USI+USI_UART
else ifeq ($(findstring pwm_test, ${C_TEST}), pwm_test)
  DEF = USE_APB0+USE_APB0_PWM
else ifeq ($(findstring map_test, ${C_TEST}), map_test)
  DEF = USE_TOP_SIM+USE_ADDR_MAP
else ifeq ($(findstring e902_had_test, ${C_TEST}), e902_had_test)
  DEF = USE_TOP_SIM+USE_HAD
endif

all: comp run
all_vip: comp_vip run_vip
all_vip_dpi: comp_vip run_vip_dpi

ccomp:
	./script/make_hex $(C_TEST)

vipccomp:
	./script/make_hex_for_vip $(C_TEST)

comp:
	vcs -debug_access -full64 -sverilog -kdb \
		-l comp.log \
		-timescale=1ns/1ps \
		+define+$(DEF) \
		-ntb_opts uvm-1.2 +define+UVM_PACKER_MAX_BYTES=1500000 +define+DEMO_MAKEFILE +define+UVM_EVENT_CALLBACK_FIX \
		-f ./soc_top/filelist/tb.f -f ./soc_top/filelist/tc.f \
		-f ./soc_top/filelist/rtl.f \
		-top tb_top

comp_vip:
	vcs -debug_access -full64 -sverilog -kdb \
		-l comp.log \
		-timescale=1ns/1ps \
		+define+USE_AHB_VIP_TO_REPLACE \
		+define+$(DEF) \
		-ntb_opts uvm-1.2 +define+UVM_PACKER_MAX_BYTES=1500000 +define+DEMO_MAKEFILE +define+UVM_EVENT_CALLBACK_FIX \
		-f ./soc_top/filelist/tb.f -f ./soc_top/filelist/tc.f \
		-f ./soc_top/filelist/rtl.f \
		-top tb_top

run: ccomp
	./simv -l simv.log +UVM_TESTNAME=soc_top_for_c_case_test

run_vip:
	./simv -l simv.log +UVM_TESTNAME=soc_top_smoke_test

run_vip_dpi: vipccomp
	./simv -l simv.log +UVM_TESTNAME=soc_top_vip_run_with_c_test -sv_lib test_start

clean:
	rm -rf verdi* comp.log simv* vc_hdrs.h csrc out hierarchy.txt ucli* tr_db.log *xml *log *fsdb novas*
	rm -rf core.* *.c_code test.pat *.so run_case.report

clean_all: clean
	rm -rf env filelist model  reg_model  sequences  tb_top  tests  uvc soc_top common
