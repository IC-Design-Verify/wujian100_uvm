`ifndef SOC_TOP_PWM_DFX_TEST__SV
`define SOC_TOP_PWM_DFX_TEST__SV

// ============================================================================
// PWM DFX/波形量测类测试 —— 与 c_case/pwm_*/ 下的 C 用例一一配合
//
// 公共模式：fork 量测/监控线程 → super.run_phase(phase)（跑 C 用例）
//           → disable fork → 断言量测结果
//
// 信号锚点（均已 RTL 实证）：
//   PAD_PWM_CH0/CH1/CH6   : tb_top 顶层 wire（DUT inout pad）
//   PAD_PWM_FAULT         : tb_top 顶层 wire，TB 不驱动（X），fault 用例需 force
//   pad_vic_int_vld[25]   : tb_top.dut.x_cpu_top.pad_vic_int_vld（core_top.v:545）
//   pwm_xx_trig           : tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_xx_trig
//   pwm_tim0_etb_trig     : tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_tim0_etb_trig
// ============================================================================

// ---------------------------------------------------------------------------
// F1(cntdiv) + F5(LOAD/CMP 占空比) —— 配合 pwm_output_duty.c
// C 侧：阶段1 LOAD=799,CMPA=200（高 75%）；阶段2 cntdiven=1,cntdiv=0
//       （RTL: cntdiv=0 → clkspec=1 → tick 每 2 pclk → 周期恰好 2 倍）
// UVM 侧：量测 CH0 所有完整周期；首周期占空比≈75%，末周期≈首周期 2 倍
// ---------------------------------------------------------------------------
class soc_top_pwm_output_duty_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_output_duty_test)

  real period_q[$];
  real high_q[$];

  extern function new(string name="soc_top_pwm_output_duty_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_output_duty_test::new(string name="soc_top_pwm_output_duty_test",
                                           uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_output_duty_test::run_phase(uvm_phase phase);
  fork
    begin : meas
      real t_r1, t_f, t_r2;
      forever begin
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r1 = $realtime;
        wait(tb_top.PAD_PWM_CH0 === 1'b0); t_f  = $realtime;
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r2 = $realtime;
        period_q.push_back(t_r2 - t_r1);
        high_q.push_back(t_f - t_r1);
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (period_q.size() < 4) begin
    `uvm_error("PWM_DUTY", $sformatf("too few periods measured: %0d", period_q.size()))
  end
  else begin
    real duty_first, ratio_last;
    duty_first = high_q[0] / period_q[0];
    ratio_last = period_q[$] / period_q[0];
    if (duty_first < 0.70 || duty_first > 0.80)
      `uvm_error("PWM_DUTY", $sformatf("phase1 duty %0.3f not ~0.75", duty_first))
    if (ratio_last < 1.8 || ratio_last > 2.2)
      `uvm_error("PWM_DUTY", $sformatf("cntdiv ratio %0.3f not ~2.0", ratio_last))
    if (duty_first >= 0.70 && duty_first <= 0.80 && ratio_last >= 1.8 && ratio_last <= 2.2)
      `uvm_info("PWM_DUTY", $sformatf("duty=%0.3f (75%%), cntdiv period ratio=%0.3f (~2x), %0d periods",
        duty_first, ratio_last, period_q.size()), UVM_LOW)
  end
endtask: run_phase

// ---------------------------------------------------------------------------
// F2: 输出极性反转 —— 配合 pwm_polarity_invert.c
// C 侧：阶段1 正常极性（高 75%）；阶段2 INVERTTRIG bit0=1（高 25%）
// UVM 侧：首周期占空比≈75%，末周期占空比≈25%
// ---------------------------------------------------------------------------
class soc_top_pwm_polarity_invert_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_polarity_invert_test)

  real period_q[$];
  real high_q[$];

  extern function new(string name="soc_top_pwm_polarity_invert_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_polarity_invert_test::new(string name="soc_top_pwm_polarity_invert_test",
                                               uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_polarity_invert_test::run_phase(uvm_phase phase);
  fork
    begin : meas
      real t_r1, t_f, t_r2;
      forever begin
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r1 = $realtime;
        wait(tb_top.PAD_PWM_CH0 === 1'b0); t_f  = $realtime;
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r2 = $realtime;
        period_q.push_back(t_r2 - t_r1);
        high_q.push_back(t_f - t_r1);
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (period_q.size() < 4) begin
    `uvm_error("PWM_INVERT", $sformatf("too few periods measured: %0d", period_q.size()))
  end
  else begin
    real duty_first, duty_last;
    duty_first = high_q[0] / period_q[0];
    duty_last  = high_q[$] / period_q[$];
    if (duty_first < 0.70 || duty_first > 0.80)
      `uvm_error("PWM_INVERT", $sformatf("phase1 duty %0.3f not ~0.75", duty_first))
    if (duty_last < 0.20 || duty_last > 0.30)
      `uvm_error("PWM_INVERT", $sformatf("phase2(inverted) duty %0.3f not ~0.25", duty_last))
    if (duty_first >= 0.70 && duty_first <= 0.80 && duty_last >= 0.20 && duty_last <= 0.30)
      `uvm_info("PWM_INVERT", $sformatf("duty %0.3f -> %0.3f (inverted, complementary)",
        duty_first, duty_last), UVM_LOW)
  end
endtask: run_phase

// ---------------------------------------------------------------------------
// F4: 计数模式 up vs up-down —— 配合 pwm_count_mode.c
// C 侧：阶段1 up（周期=LOAD+1）；阶段2 up-down（周期=2*(LOAD-1)≈2x）
// UVM 侧：末周期 / 首周期 ≈ 2
// ---------------------------------------------------------------------------
class soc_top_pwm_count_mode_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_count_mode_test)

  real period_q[$];

  extern function new(string name="soc_top_pwm_count_mode_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_count_mode_test::new(string name="soc_top_pwm_count_mode_test",
                                          uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_count_mode_test::run_phase(uvm_phase phase);
  fork
    begin : meas
      real t_r1, t_r2;
      forever begin
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r1 = $realtime;
        wait(tb_top.PAD_PWM_CH0 === 1'b0);
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r2 = $realtime;
        period_q.push_back(t_r2 - t_r1);
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (period_q.size() < 4) begin
    `uvm_error("PWM_CNT_MODE", $sformatf("too few periods measured: %0d", period_q.size()))
  end
  else begin
    real ratio;
    ratio = period_q[$] / period_q[0];
    if (ratio < 1.8 || ratio > 2.2)
      `uvm_error("PWM_CNT_MODE", $sformatf("up-down/up period ratio %0.3f not ~2.0", ratio))
    else
      `uvm_info("PWM_CNT_MODE", $sformatf("up-down/up period ratio=%0.3f (~2x), %0d periods",
        ratio, period_q.size()), UVM_LOW)
  end
endtask: run_phase

// ---------------------------------------------------------------------------
// F6: 死区 —— 配合 pwm_deadband.c
// C 侧：group0 CH0/CH1 互补对 + db0en + delay0=0x10
// UVM 侧：CH0/CH1 均翻转、任何沿时刻两路不同时为高（死区无重叠）
// ---------------------------------------------------------------------------
class soc_top_pwm_deadband_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_deadband_test)

  int ch0_tog = 0;
  int ch1_tog = 0;
  bit overlap = 0;

  extern function new(string name="soc_top_pwm_deadband_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_deadband_test::new(string name="soc_top_pwm_deadband_test",
                                        uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_deadband_test::run_phase(uvm_phase phase);
  fork
    begin : tog0
      forever begin
        wait(tb_top.PAD_PWM_CH0 === 1'b1); ch0_tog++;
        wait(tb_top.PAD_PWM_CH0 === 1'b0);
      end
    end
    begin : tog1
      forever begin
        wait(tb_top.PAD_PWM_CH1 === 1'b1); ch1_tog++;
        wait(tb_top.PAD_PWM_CH1 === 1'b0);
      end
    end
    begin : ovl
      forever begin
        @(tb_top.PAD_PWM_CH0 or tb_top.PAD_PWM_CH1);
        if (tb_top.PAD_PWM_CH0 === 1'b1 && tb_top.PAD_PWM_CH1 === 1'b1)
          overlap = 1;
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (ch0_tog < 5)  `uvm_error("PWM_DB", $sformatf("CH0 toggles too few: %0d", ch0_tog))
  if (ch1_tog < 5)  `uvm_error("PWM_DB", $sformatf("CH1 toggles too few: %0d", ch1_tog))
  if (overlap)      `uvm_error("PWM_DB", "CH0/CH1 overlap detected (deadband violated)")
  if (ch0_tog >= 5 && ch1_tog >= 5 && !overlap)
    `uvm_info("PWM_DB", $sformatf("deadband ok: CH0 toggles=%0d CH1 toggles=%0d no overlap",
      ch0_tog, ch1_tog), UVM_LOW)
endtask: run_phase

// ---------------------------------------------------------------------------
// F10: fault 中断 —— 配合 pwm_fault.c
// RTL: int_fault <= 1 当 (fault & intenfault)（电平与，pwm.v:4718）
// UVM 侧：run_phase 起始 force PAD_PWM_FAULT=0（消 X，必须先于 C 置 INTEN），
//         C 启动完成绰绰有余后 force 1 注入 fault，保持到结束
// C 侧：INTEN1[0]=1 → 轮询 PWMRIS1 bit0 → PWMIC1 清除
// ---------------------------------------------------------------------------
class soc_top_pwm_fault_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_fault_test)

  extern function new(string name="soc_top_pwm_fault_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_fault_test::new(string name="soc_top_pwm_fault_test",
                                     uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_fault_test::run_phase(uvm_phase phase);
  fork
    begin : fault_inj
      force tb_top.PAD_PWM_FAULT = 1'b0;   // 立即消 X
      #1ms;                                // 等 C 完成 boot + INTEN 配置（轮询超时 200000 次兜底）
      force tb_top.PAD_PWM_FAULT = 1'b1;   // 注入 fault
    end
  join_none

  super.run_phase(phase);

  disable fork;
  release tb_top.PAD_PWM_FAULT;
  `uvm_info("PWM_FAULT", "fault injected via force, C observed+cleared PWMRIS1 bit0", UVM_LOW)
endtask: run_phase

// ---------------------------------------------------------------------------
// F14: PWM 中断 VIC 路由 —— 配合 pwm_vic_route.c
// C 侧：group0 cnt_zero 中断（INTEN1 bit8）断言→保持→清除→保持
// UVM 侧：pad_vic_int_vld[25] 先见 1 后见 0
// ---------------------------------------------------------------------------
class soc_top_pwm_vic_route_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_vic_route_test)

  bit seen_assert;
  bit seen_deassert;

  extern function new(string name="soc_top_pwm_vic_route_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_vic_route_test::new(string name="soc_top_pwm_vic_route_test",
                                         uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_vic_route_test::run_phase(uvm_phase phase);
  fork
    begin : vic_monitor
      wait(tb_top.dut.x_cpu_top.pad_vic_int_vld[25] === 1'b1);
      seen_assert = 1'b1;
      wait(tb_top.dut.x_cpu_top.pad_vic_int_vld[25] === 1'b0);
      seen_deassert = 1'b1;
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (!seen_assert)
    `uvm_error("PWM_VIC_ROUTE", "pwm_int route missing: pad_vic_int_vld[25] never asserted")
  if (!seen_deassert)
    `uvm_error("PWM_VIC_ROUTE", "pwm_int route clear missing: pad_vic_int_vld[25] never deasserted")
  if (seen_assert && seen_deassert)
    `uvm_info("PWM_VIC_ROUTE", "pwm_int routed to pad_vic_int_vld[25], cleared after PWMIC1", UVM_LOW)
endtask: run_phase

// ---------------------------------------------------------------------------
// F3 + F11: 触发输出 / ETB 触发输出 —— 配合 pwm_trig_etb.c
// RTL: trigger=(cnt==trigval 沿)&trenu（INVERTTRIG bit13）；pwm_xx_trig=OR(trigger0..5)
//      pwm_tim0_etb_trig=tim_cnt_match_flag_divedge（1 pclk 脉冲）
// UVM 侧：wait(===) 数脉冲（窄脉冲必须 level-sensitive wait，不可 @posedge）
// 注：F11 ETB 触发输入在 apb0_sub_top tie 0，无法激励（记录于报告）
// ---------------------------------------------------------------------------
class soc_top_pwm_trig_etb_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_trig_etb_test)

  int trig_cnt = 0;
  int tim_etb_cnt = 0;

  extern function new(string name="soc_top_pwm_trig_etb_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_trig_etb_test::new(string name="soc_top_pwm_trig_etb_test",
                                        uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_trig_etb_test::run_phase(uvm_phase phase);
  fork
    begin : trig_mon
      forever begin
        wait(tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_xx_trig === 1'b1);
        trig_cnt++;
        wait(tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_xx_trig === 1'b0);
      end
    end
    begin : tim_mon
      forever begin
        wait(tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_tim0_etb_trig === 1'b1);
        tim_etb_cnt++;
        wait(tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_tim0_etb_trig === 1'b0);
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (trig_cnt < 1)
    `uvm_error("PWM_TRIG", "pwm_xx_trig never pulsed (trigger output F3)")
  if (tim_etb_cnt < 1)
    `uvm_error("PWM_TRIG", "pwm_tim0_etb_trig never pulsed (ETB trig output F11)")
  if (trig_cnt >= 1 && tim_etb_cnt >= 1)
    `uvm_info("PWM_TRIG", $sformatf("pwm_xx_trig pulses=%0d, pwm_tim0_etb_trig pulses=%0d",
      trig_cnt, tim_etb_cnt), UVM_LOW)
endtask: run_phase

// ---------------------------------------------------------------------------
// F13: 多组独立配置 —— 配合 pwm_multi_group.c
// C 侧：group0 LOAD=0x100（CH0），group3 LOAD=0x400（CH6）
// UVM 侧：CH6 周期 / CH0 周期 ≈ 4
// ---------------------------------------------------------------------------
class soc_top_pwm_multi_group_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_pwm_multi_group_test)

  real ch0_period_q[$];
  real ch6_period_q[$];

  extern function new(string name="soc_top_pwm_multi_group_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_pwm_multi_group_test::new(string name="soc_top_pwm_multi_group_test",
                                           uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_pwm_multi_group_test::run_phase(uvm_phase phase);
  fork
    begin : meas0
      real t_r1, t_r2;
      forever begin
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r1 = $realtime;
        wait(tb_top.PAD_PWM_CH0 === 1'b0);
        wait(tb_top.PAD_PWM_CH0 === 1'b1); t_r2 = $realtime;
        ch0_period_q.push_back(t_r2 - t_r1);
      end
    end
    begin : meas6
      real t_r1, t_r2;
      forever begin
        wait(tb_top.PAD_PWM_CH6 === 1'b1); t_r1 = $realtime;
        wait(tb_top.PAD_PWM_CH6 === 1'b0);
        wait(tb_top.PAD_PWM_CH6 === 1'b1); t_r2 = $realtime;
        ch6_period_q.push_back(t_r2 - t_r1);
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (ch0_period_q.size() < 3 || ch6_period_q.size() < 3) begin
    `uvm_error("PWM_MGRP", $sformatf("too few periods: CH0=%0d CH6=%0d",
      ch0_period_q.size(), ch6_period_q.size()))
  end
  else begin
    real ratio;
    ratio = ch6_period_q[$] / ch0_period_q[$];
    if (ratio < 3.8 || ratio > 4.2)
      `uvm_error("PWM_MGRP", $sformatf("group3/group0 period ratio %0.3f not ~4.0", ratio))
    else
      `uvm_info("PWM_MGRP", $sformatf("independent groups ok: CH6/CH0 period ratio=%0.3f (~4x)",
        ratio), UVM_LOW)
  end
endtask: run_phase

`endif
