# wujian100_uvm
使用github上开源基于UVM的ahb_agent/apb_agent，搭建无剑100的UVM验证环境，供大家学习；有兴趣的同学也可以一起加入完善这个验证环境。

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
	# 仿真
	make

# 微信公众号
相关环境介绍可以扫码关注我的微信公众号查看（带SVT VIP的UVM验证环境介绍）。
<img src="https://github.com/IC-Design-Verify/wujian100_uvm/blob/main/images/wxgzh.jpg" width="485" height="640" alt="wxgzh" align=center/>
