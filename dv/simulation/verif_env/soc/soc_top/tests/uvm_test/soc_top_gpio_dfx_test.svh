`ifndef SOC_TOP_GPIO_DFX_TEST__SV
`define SOC_TOP_GPIO_DFX_TEST__SV

// F12: GPIO 中断路由——配合 c_case/gpio_vic_route/gpio_vic_route.c
// C 侧：level-high + 全使能 → 断言；inten=0 → 撤销
// UVM 侧：轮询 pad_vic_int_vld[16]（core_top.v:540 ip_cpu_int_vld[16]=gpio_wic_intr）
class soc_top_gpio_vic_route_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_gpio_vic_route_test)

  bit seen_bit16;
  bit seen_bit16_clear;

  extern function new(string name="soc_top_gpio_vic_route_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_gpio_vic_route_test::new(string name="soc_top_gpio_vic_route_test",
                                           uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_gpio_vic_route_test::run_phase(uvm_phase phase);
  uvm_hdl_data_t vic_val;
  fork
    begin : vic_monitor
      while (1) begin
        #20ns;
        if (!uvm_hdl_read("tb_top.dut.x_cpu_top.pad_vic_int_vld", vic_val)) begin
          `uvm_error("GPIO_VIC_ROUTE",
            "uvm_hdl_read failed for tb_top.dut.x_cpu_top.pad_vic_int_vld")
        end
        else begin
          if (vic_val[16]) seen_bit16 = 1'b1;
          else if (seen_bit16) seen_bit16_clear = 1'b1;
        end
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (!seen_bit16)
    `uvm_error("GPIO_VIC_ROUTE",
      "GPIO route missing: pad_vic_int_vld[16] never asserted")
  if (!seen_bit16_clear)
    `uvm_error("GPIO_VIC_ROUTE",
      "GPIO route clear missing: pad_vic_int_vld[16] never deasserted after inten=0")
  if (seen_bit16 && seen_bit16_clear)
    `uvm_info("GPIO_VIC_ROUTE",
      "GPIO routed to pad_vic_int_vld[16], cleared after inten=0", UVM_LOW)
endtask: run_phase

// F11: GPIO ETB 触发——配合 c_case/gpio_etb_trig/gpio_etb_trig.c
// RTL: gpio0_etb_trig = int_level（极性调整后 PAD 电平，pclk_int 2-flop 同步）
// C 侧：pol=high → 0x55555555；pol=low → 0xAAAAAAAA
// UVM 侧：轮询 XMR 确认两个值依序出现
// 注：gpio0_etb_trig 在 wujian100_open_top 未引出（aou_top 内部线网，无 ETB 消费者）
class soc_top_gpio_etb_trig_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_gpio_etb_trig_test)

  bit seen_55;
  bit seen_aa;

  extern function new(string name="soc_top_gpio_etb_trig_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_gpio_etb_trig_test::new(string name="soc_top_gpio_etb_trig_test",
                                          uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_gpio_etb_trig_test::run_phase(uvm_phase phase);
  uvm_hdl_data_t etb_val;
  fork
    begin : etb_monitor
      while (1) begin
        #20ns;
        if (!uvm_hdl_read("tb_top.dut.x_aou_top.gpio0_etb_trig", etb_val)) begin
          `uvm_error("GPIO_ETB_TRIG",
            "uvm_hdl_read failed for tb_top.dut.x_aou_top.gpio0_etb_trig")
        end
        else begin
          if (etb_val[31:0] == 32'h55555555) seen_55 = 1'b1;
          if (seen_55 && etb_val[31:0] == 32'hAAAAAAAA) seen_aa = 1'b1;
        end
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (!seen_55)
    `uvm_error("GPIO_ETB_TRIG",
      "gpio0_etb_trig never showed 0x55555555 (pol=high phase)")
  if (!seen_aa)
    `uvm_error("GPIO_ETB_TRIG",
      "gpio0_etb_trig never showed 0xAAAAAAAA after 0x55555555 (pol=low phase)")
  if (seen_55 && seen_aa)
    `uvm_info("GPIO_ETB_TRIG",
      "gpio0_etb_trig followed int_level: 0x55555555 then 0xAAAAAAAA", UVM_LOW)
endtask: run_phase

`endif
