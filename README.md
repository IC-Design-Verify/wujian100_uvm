# wujian100_uvm
使用github上开源基于UVM的ahb_agent/apb_agent，搭建无剑100的UVM验证环境，供大家学习；有兴趣的同学也可以一起加入完善这个验证环境。

# 微信公众号
相关环境介绍可以扫码关注我的微信公众号查看（带SVT VIP的UVM验证环境介绍）。
<img src="https://github.com/IC-Design-Verify/wujian100_uvm/blob/main/images/wxgzh.jpg" width="485" height="640" alt="wxgzh" align=center/>

# Requirement
其中CPU用例需要用到gcc工具链进行编译，这里提供给大家gcc工具链的下载链接，推荐解压在/common/riscv-toolchain目录下；

否则需要修改dv.cshrc中的tool环境变量到自己解压的路径下

通过网盘分享的文件：riscv64-elf-x86_64-20210512.tar.gz
链接: https://pan.baidu.com/s/1vikbNSVANvInTjVqMAXUHQ?pwd=ye2s 提取码: ye2s 复制这段内容后打开百度网盘手机App，操作更方便哦 

# Steps
	# 下载仓库及子仓库
	git clone --recurse-submodule https://github.com/IC-Design-Verify/wujian100_uvm.git
	# 到dv目录
	cd wujian100_uvm/dv
	# 切换环境到csh/tcsh
	csh
	# 设置环境变量
	source dv.cshrc
	# 进入soc环境目录
	cd simulation/verif_env/soc
## 仿真
### CPU驱动的仿真：可以编译仿真, 默认仿真timer/timer_test.c用例
	make all
### VIP驱动的仿真：可以编译仿真，已实现smoke用例跑timer的功能
	make all_vip
### VIP驱动的仿真（DPI调用CPU的C case）：可以编译仿真，默认仿真timer/timer_test.c用例
	make all_vip_dpi

