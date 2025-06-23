# Filename:  soc_top.py
# Brief:     
# Author:    IC_verify
# Version:   v1.00
# Data:      2024-03-24


from ksim_common import *

def_build(build='soc_top', base_build='soc', owner='IC_verify')
set_sticky_attrib(build='soc_top', owner='IC_verify')

def_test(
    'soc_top_smoke',
    '+UVM_TESTNAME=soc_smoke_test',
    timeout=18000,
    fixed_seed = 1798512,
    #c_code_opts = '-c_case_test=cpu_mini_smoke -c_define=ENABLE_CPU_FUSA',
)







# Below is an example

#def_build(build='build1', base_build='soc', owner='IC_verify')
#
#def_build(build='build2', base_build='soc', owner='IC_verify')
#
#set_sticky_attrib(build='build1', owner='IC_verify', timeout=20000)
#
#first_base_opts =  ' +UVM_TESTNAME=soc_cpu_base \
#                     +Force_Trustzone +Force_Boot     '      
#
#
#def_test(
#    'xxx_smoke',
#    first_base_opts,
#    timeout=18000,
#    fixed_seed = 1798512,
#    #c_code_opts = '-sys_name=cpu_ss  -c_case_test=cpu_mini_smoke ',
#)
#
#set_sticky_attrib(build='build2', owner='IC_verify')
#second_base_opt = '+UVM_TESTNAME=soc_cpu_real_lpddr_base\
#                         +fsdb+force  '                         
#def_test(
#    'yyy_smoke',
#    second_base_opt,
#    #c_code_opts = '-sys_name=cpu_ss -c_case_test=cpu_ss_ddr0_smoke ',
#)

