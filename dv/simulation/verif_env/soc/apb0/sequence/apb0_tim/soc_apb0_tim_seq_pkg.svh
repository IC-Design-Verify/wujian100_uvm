`ifndef SOC_SUBSYS_APB0_TIM_SEQ_PKG__SV
`define SOC_SUBSYS_APB0_TIM_SEQ_PKG__SV

  import ahb_pkg::*;

  `include "soc_top_v_sequence_pkg.svh"
  `include "soc_subsys_apb0_seq_pkg.svh"

  // Timer checker (reference model + self-checking scoreboard-style)
  `include "apb0_tim/soc_apb0_tim_checker.svh"

  // Timer base sequence (AHB helper tasks)
  `include "apb0_tim/soc_apb0_tim_sequence.svh"

  // Timer virtual sequence (smoke scenario)
  `include "apb0_tim/soc_apb0_tim_vseq.svh"

  // P1 register-access virtual sequence
  `include "apb0_tim/soc_apb0_tim_reg_vseq.svh"

  // P1 boundary + interrupt virtual sequence
  `include "apb0_tim/soc_apb0_tim_bound_vseq.svh"

  // P1 concurrency virtual sequence (dual timer)
  `include "apb0_tim/soc_apb0_tim_concur_vseq.svh"

`endif
