$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/tb_top/soc_top_env_define.sv
//////////////////////////////
//sub_env directory
//////////////////////////////
//sub_env filelist


//////////////////////////////
//uvc filelist
-f $PROJ_HOME/dv/simulation/verif_env/soc/soc_top/filelist/ahb.f

//////////////////////////////
//env directory
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/env
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/env/reference_model
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/env/scoreboard
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/reg_model
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/sequences
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/sequences/reg_sequence
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/sequences/intr_sequence
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/tests/uvm_test
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/headers
+incdir+$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/tb_top

//////////////////////////////
//interface
$PROJ_HOME/dv/simulation/verif_env/soc/common/uvc/clock_if.sv
$PROJ_HOME/dv/simulation/verif_env/soc/common/uvc/intr_if.sv
$PROJ_HOME/dv/simulation/verif_env/soc/common/uvc/reset_if.sv

//////////////////////////////
//env package
$PROJ_HOME/dv/simulation/verif_env/soc/soc_top/env/soc_top_env_pkg.sv
