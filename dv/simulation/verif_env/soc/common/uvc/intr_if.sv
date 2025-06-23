`ifndef INTR_IF__SV
`define INTR_IF__SV
interface intr_if(input clk, input rst_n);
  logic[63:0] IRQ;

  task wait_for_interrupt_0();
    @(posedge IRQ[0]);
  endtask: wait_for_interrupt_0

  task wait_for_interrupt_1();
    @(posedge IRQ[1]);
  endtask: wait_for_interrupt_1

  task wait_for_interrupt_2();
    @(posedge IRQ[2]);
  endtask: wait_for_interrupt_2

  task wait_for_interrupt_3();
    @(posedge IRQ[3]);
  endtask: wait_for_interrupt_3

  task wait_for_interrupt_4();
    @(posedge IRQ[4]);
  endtask: wait_for_interrupt_4

  task wait_for_interrupt_5();
    @(posedge IRQ[5]);
  endtask: wait_for_interrupt_5

  task wait_for_interrupt_6();
    @(posedge IRQ[6]);
  endtask: wait_for_interrupt_6

  task wait_for_interrupt_7();
    @(posedge IRQ[7]);
  endtask: wait_for_interrupt_7

  task wait_for_interrupt_8();
    @(posedge IRQ[8]);
  endtask: wait_for_interrupt_8

  task wait_for_interrupt_9();
    @(posedge IRQ[9]);
  endtask: wait_for_interrupt_9

  task wait_for_interrupt_10();
    @(posedge IRQ[10]);
  endtask: wait_for_interrupt_10

  task wait_for_interrupt_11();
    @(posedge IRQ[11]);
  endtask: wait_for_interrupt_11

  task wait_for_interrupt_12();
    @(posedge IRQ[12]);
  endtask: wait_for_interrupt_12

  task wait_for_interrupt_13();
    @(posedge IRQ[13]);
  endtask: wait_for_interrupt_13

  task wait_for_interrupt_14();
    @(posedge IRQ[14]);
  endtask: wait_for_interrupt_14

  task wait_for_interrupt_15();
    @(posedge IRQ[15]);
  endtask: wait_for_interrupt_15

  task wait_for_interrupt_16();
    @(posedge IRQ[16]);
  endtask: wait_for_interrupt_16

  task wait_for_interrupt_17();
    @(posedge IRQ[17]);
  endtask: wait_for_interrupt_17

  task wait_for_interrupt_18();
    @(posedge IRQ[18]);
  endtask: wait_for_interrupt_18

  task wait_for_interrupt_19();
    @(posedge IRQ[19]);
  endtask: wait_for_interrupt_19

  task wait_for_interrupt_20();
    @(posedge IRQ[20]);
  endtask: wait_for_interrupt_20

  task wait_for_interrupt_21();
    @(posedge IRQ[21]);
  endtask: wait_for_interrupt_21

  task wait_for_interrupt_22();
    @(posedge IRQ[22]);
  endtask: wait_for_interrupt_22

  task wait_for_interrupt_23();
    @(posedge IRQ[23]);
  endtask: wait_for_interrupt_23

  task wait_for_interrupt_24();
    @(posedge IRQ[24]);
  endtask: wait_for_interrupt_24

  task wait_for_interrupt_25();
    @(posedge IRQ[25]);
  endtask: wait_for_interrupt_25

  task wait_for_interrupt_26();
    @(posedge IRQ[26]);
  endtask: wait_for_interrupt_26

  task wait_for_interrupt_27();
    @(posedge IRQ[27]);
  endtask: wait_for_interrupt_27

  task wait_for_interrupt_28();
    @(posedge IRQ[28]);
  endtask: wait_for_interrupt_28

  task wait_for_interrupt_29();
    @(posedge IRQ[29]);
  endtask: wait_for_interrupt_29

  task wait_for_interrupt_30();
    @(posedge IRQ[30]);
  endtask: wait_for_interrupt_30

  task wait_for_interrupt_31();
    @(posedge IRQ[31]);
  endtask: wait_for_interrupt_31

  task wait_for_interrupt_32();
    @(posedge IRQ[32]);
  endtask: wait_for_interrupt_32

  task wait_for_interrupt_33();
    @(posedge IRQ[33]);
  endtask: wait_for_interrupt_33

  task wait_for_interrupt_34();
    @(posedge IRQ[34]);
  endtask: wait_for_interrupt_34

  task wait_for_interrupt_35();
    @(posedge IRQ[35]);
  endtask: wait_for_interrupt_35

  task wait_for_interrupt_36();
    @(posedge IRQ[36]);
  endtask: wait_for_interrupt_36

  task wait_for_interrupt_37();
    @(posedge IRQ[37]);
  endtask: wait_for_interrupt_37

  task wait_for_interrupt_38();
    @(posedge IRQ[38]);
  endtask: wait_for_interrupt_38

  task wait_for_interrupt_39();
    @(posedge IRQ[39]);
  endtask: wait_for_interrupt_39

  task wait_for_interrupt_40();
    @(posedge IRQ[40]);
  endtask: wait_for_interrupt_40

  task wait_for_interrupt_41();
    @(posedge IRQ[41]);
  endtask: wait_for_interrupt_41

  task wait_for_interrupt_42();
    @(posedge IRQ[42]);
  endtask: wait_for_interrupt_42

  task wait_for_interrupt_43();
    @(posedge IRQ[43]);
  endtask: wait_for_interrupt_43

  task wait_for_interrupt_44();
    @(posedge IRQ[44]);
  endtask: wait_for_interrupt_44

  task wait_for_interrupt_45();
    @(posedge IRQ[45]);
  endtask: wait_for_interrupt_45

  task wait_for_interrupt_46();
    @(posedge IRQ[46]);
  endtask: wait_for_interrupt_46

  task wait_for_interrupt_47();
    @(posedge IRQ[47]);
  endtask: wait_for_interrupt_47

  task wait_for_interrupt_48();
    @(posedge IRQ[48]);
  endtask: wait_for_interrupt_48

  task wait_for_interrupt_49();
    @(posedge IRQ[49]);
  endtask: wait_for_interrupt_49

  task wait_for_interrupt_50();
    @(posedge IRQ[50]);
  endtask: wait_for_interrupt_50

  task wait_for_interrupt_51();
    @(posedge IRQ[51]);
  endtask: wait_for_interrupt_51

  task wait_for_interrupt_52();
    @(posedge IRQ[52]);
  endtask: wait_for_interrupt_52

  task wait_for_interrupt_53();
    @(posedge IRQ[53]);
  endtask: wait_for_interrupt_53

  task wait_for_interrupt_54();
    @(posedge IRQ[54]);
  endtask: wait_for_interrupt_54

  task wait_for_interrupt_55();
    @(posedge IRQ[55]);
  endtask: wait_for_interrupt_55

  task wait_for_interrupt_56();
    @(posedge IRQ[56]);
  endtask: wait_for_interrupt_56

  task wait_for_interrupt_57();
    @(posedge IRQ[57]);
  endtask: wait_for_interrupt_57

  task wait_for_interrupt_58();
    @(posedge IRQ[58]);
  endtask: wait_for_interrupt_58

  task wait_for_interrupt_59();
    @(posedge IRQ[59]);
  endtask: wait_for_interrupt_59

  task wait_for_interrupt_60();
    @(posedge IRQ[60]);
  endtask: wait_for_interrupt_60

  task wait_for_interrupt_61();
    @(posedge IRQ[61]);
  endtask: wait_for_interrupt_61

  task wait_for_interrupt_62();
    @(posedge IRQ[62]);
  endtask: wait_for_interrupt_62

  task wait_for_interrupt_63();
    @(posedge IRQ[63]);
  endtask: wait_for_interrupt_63

  function bit is_interrupt_0_cleared();
    if(IRQ[0]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_0_cleared

  function bit is_interrupt_1_cleared();
    if(IRQ[1]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_1_cleared

  function bit is_interrupt_2_cleared();
    if(IRQ[2]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_2_cleared

  function bit is_interrupt_3_cleared();
    if(IRQ[3]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_3_cleared

  function bit is_interrupt_4_cleared();
    if(IRQ[4]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_4_cleared

  function bit is_interrupt_5_cleared();
    if(IRQ[5]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_5_cleared

  function bit is_interrupt_6_cleared();
    if(IRQ[6]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_6_cleared

  function bit is_interrupt_7_cleared();
    if(IRQ[7]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_7_cleared

  function bit is_interrupt_8_cleared();
    if(IRQ[8]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_8_cleared

  function bit is_interrupt_9_cleared();
    if(IRQ[9]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_9_cleared

  function bit is_interrupt_10_cleared();
    if(IRQ[10]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_10_cleared

  function bit is_interrupt_11_cleared();
    if(IRQ[11]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_11_cleared

  function bit is_interrupt_12_cleared();
    if(IRQ[12]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_12_cleared

  function bit is_interrupt_13_cleared();
    if(IRQ[13]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_13_cleared

  function bit is_interrupt_14_cleared();
    if(IRQ[14]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_14_cleared

  function bit is_interrupt_15_cleared();
    if(IRQ[15]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_15_cleared

  function bit is_interrupt_16_cleared();
    if(IRQ[16]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_16_cleared

  function bit is_interrupt_17_cleared();
    if(IRQ[17]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_17_cleared

  function bit is_interrupt_18_cleared();
    if(IRQ[18]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_18_cleared

  function bit is_interrupt_19_cleared();
    if(IRQ[19]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_19_cleared

  function bit is_interrupt_20_cleared();
    if(IRQ[20]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_20_cleared

  function bit is_interrupt_21_cleared();
    if(IRQ[21]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_21_cleared

  function bit is_interrupt_22_cleared();
    if(IRQ[22]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_22_cleared

  function bit is_interrupt_23_cleared();
    if(IRQ[23]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_23_cleared

  function bit is_interrupt_24_cleared();
    if(IRQ[24]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_24_cleared

  function bit is_interrupt_25_cleared();
    if(IRQ[25]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_25_cleared

  function bit is_interrupt_26_cleared();
    if(IRQ[26]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_26_cleared

  function bit is_interrupt_27_cleared();
    if(IRQ[27]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_27_cleared

  function bit is_interrupt_28_cleared();
    if(IRQ[28]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_28_cleared

  function bit is_interrupt_29_cleared();
    if(IRQ[29]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_29_cleared

  function bit is_interrupt_30_cleared();
    if(IRQ[30]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_30_cleared

  function bit is_interrupt_31_cleared();
    if(IRQ[31]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_31_cleared

  function bit is_interrupt_32_cleared();
    if(IRQ[32]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_32_cleared

  function bit is_interrupt_33_cleared();
    if(IRQ[33]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_33_cleared

  function bit is_interrupt_34_cleared();
    if(IRQ[34]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_34_cleared

  function bit is_interrupt_35_cleared();
    if(IRQ[35]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_35_cleared

  function bit is_interrupt_36_cleared();
    if(IRQ[36]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_36_cleared

  function bit is_interrupt_37_cleared();
    if(IRQ[37]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_37_cleared

  function bit is_interrupt_38_cleared();
    if(IRQ[38]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_38_cleared

  function bit is_interrupt_39_cleared();
    if(IRQ[39]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_39_cleared

  function bit is_interrupt_40_cleared();
    if(IRQ[40]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_40_cleared

  function bit is_interrupt_41_cleared();
    if(IRQ[41]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_41_cleared

  function bit is_interrupt_42_cleared();
    if(IRQ[42]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_42_cleared

  function bit is_interrupt_43_cleared();
    if(IRQ[43]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_43_cleared

  function bit is_interrupt_44_cleared();
    if(IRQ[44]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_44_cleared

  function bit is_interrupt_45_cleared();
    if(IRQ[45]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_45_cleared

  function bit is_interrupt_46_cleared();
    if(IRQ[46]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_46_cleared

  function bit is_interrupt_47_cleared();
    if(IRQ[47]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_47_cleared

  function bit is_interrupt_48_cleared();
    if(IRQ[48]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_48_cleared

  function bit is_interrupt_49_cleared();
    if(IRQ[49]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_49_cleared

  function bit is_interrupt_50_cleared();
    if(IRQ[50]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_50_cleared

  function bit is_interrupt_51_cleared();
    if(IRQ[51]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_51_cleared

  function bit is_interrupt_52_cleared();
    if(IRQ[52]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_52_cleared

  function bit is_interrupt_53_cleared();
    if(IRQ[53]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_53_cleared

  function bit is_interrupt_54_cleared();
    if(IRQ[54]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_54_cleared

  function bit is_interrupt_55_cleared();
    if(IRQ[55]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_55_cleared

  function bit is_interrupt_56_cleared();
    if(IRQ[56]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_56_cleared

  function bit is_interrupt_57_cleared();
    if(IRQ[57]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_57_cleared

  function bit is_interrupt_58_cleared();
    if(IRQ[58]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_58_cleared

  function bit is_interrupt_59_cleared();
    if(IRQ[59]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_59_cleared

  function bit is_interrupt_60_cleared();
    if(IRQ[60]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_60_cleared

  function bit is_interrupt_61_cleared();
    if(IRQ[61]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_61_cleared

  function bit is_interrupt_62_cleared();
    if(IRQ[62]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_62_cleared

  function bit is_interrupt_63_cleared();
    if(IRQ[63]== 0)
      return 1;
    else
      return 0;
  endfunction: is_interrupt_63_cleared

  task wait_n_cycle(int n);
    @(posedge clk);
    assert(n>0);
    repeat(n-1) @(posedge clk);
  endtask: wait_n_cycle
endinterface: intr_if
`endif
