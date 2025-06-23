# tool path
setenv TOOL_PATH                /common/riscv-toolchain
# lib path

# KSIM requirement
setenv KSIM_SIMULATOR           vcs
setenv KSIM_PWA_BASE            /home/IC_verify/project
setenv KSIM_PROJECT             wujian100_uvm
setenv KSIM_TB_REL_PATH         dv
setenv KSIM_SIM_DIR             /home/IC_verify/simdir
setenv KSIM_COMP_OPTS           -full64
setenv KSIM_SIM_OPTS            ""

setenv PROJ_HOME            `pwd | sed -e 's/^\(.*\)\/dv\>.*/\1/'`
