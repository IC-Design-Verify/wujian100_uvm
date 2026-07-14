# 覆盖率收集与统计分析规范

> 基于 logic_op IP 覆盖率回归实践总结
> v1.1 — 2026-06-16（VCS W-2024.09-SP1 URG segfault 已修复）

## 版本说明

- v1.0 (2026-05-06)：初版，**记录** VCS W-2024.09-SP1 上 URG 的两类 segfault 现象与"重试/避用 merge"等临时绕过方案。
- v1.1 (2026-06-16)：**VCS 工具层修复完成**（URG 不再 segfault），文档同步更新为正常使用流程。原先的"重新运行 urg 直到没有报错"、"用单一 vdb 代替 merge"、"rm -rf 输出目录"等 workaround **全部作废**，按本文档的"标准流程"操作即可。

> ⚠️ **重要变更**：v1.0 中所有"URG 崩溃 → 重试或换方法"的建议均已过时。**VCS/URG 工具已修复**，按标准命令直接运行即可，无需任何 workaround。

---

## 目录

1. [环境设置](#env-setup)
2. [ksim --code_cov 的正确用法](#ksim-code-cov)
3. [编译文件中显式添加 -cm 标志](#cm-flags)
4. [URG 报告生成](#urg-report)
5. [覆盖率数据解读](#coverage-analysis)
6. [常见问题与解决方案](#troubleshooting)
7. [自检清单](#checklist)

---

<a id="env-setup"></a>
## 1. 环境设置

```bash
# 必须按顺序执行，使用绝对路径
cd /home/IC_verify/project/wujian100/dv && source dv.bashrc
source /home/IC_verify/project/wujian100/.claude/skills/ksim/scripts/sourceme.bash

# PYTHONPATH 顺序关键：pylib 必须在最前面
export PYTHONPATH="/home/IC_verify/project/wujian100/.claude/skills/ksim/scripts/mypylib/pylib:/home/IC_verify/project/wujian100/.claude/skills/ksim/scripts/mypylib/lib:/home/IC_verify/project/wujian100/.claude/skills/ksim/scripts"
export KSIM_DB_DIR=/home/IC_verify
```

<a id="ksim-code-cov"></a>
## 2. ksim --code_cov 的正确用法

### 核心发现

`--code_cov` shortcut 的行为：
- 编译时添加 `KSIM_CMD_COMP_OPTS` 中的 `-cm` 标志
- 仿真时添加 `-cm` 标志到 simv 命令行
- **但 Mako 模板 `vcs_default_opts.file` 不会自动包含 `-cm` 标志**

`--code_cov` 在 `plugin/vcs.yml` 中的定义：
```yaml
- name: --code_cov
  args: ['--timeout', 'x5', '--comp', '-cm line+cond+fsm+tgl+branch+assert -cm_noconst -cm_seqnoconst -cm_line contassign -cm_tgl mda+structarr+fullintf -cm_cond obs ', '--sim', '-cm line+cond+fsm+tgl+branch+assert ']
```

### 必须执行的命令

```bash
# 完整编译+仿真（必须同时做，不能分开 -co + -so）
ksim -t logic_op::logic_op.py --code_cov --clean

# --code_cov 触发完整重编译（--clean 确保无旧产物干扰）
# 不能用 -so 单独添加覆盖率（覆盖率是编译时选项）
```

### 模板修改（如果默认不生效）

如果 `--code_cov` 的 `-cm` 标志没有进入编译命令行，需要在 `vcs_logic_op.file` 模板中显式添加：

```python
# dv/simulation/verif_env/ip/<module>/build/vcs_<module>.file

-debug_access+r+f
-timescale=1ns/1ps
-assert svaext

# Coverage options — 显式添加确保编译时启用
-cm line+cond+fsm+tgl+branch+assert
-cm_noconst
-cm_seqnoconst
-cm_line contassign
-cm_tgl mda+structarr+fullintf
-cm_cond obs

+define+SVT_APB_MAX_DATA_WIDTH=32
...
```

<a id="cm-flags"></a>
## 3. -cm 标志说明

| 标志 | 含义 | 是否必须 |
|------|------|----------|
| `-cm line+cond` | 行+条件覆盖率（最小集合） | **必须** |
| `-cm fsm` | FSM 状态覆盖率 | 推荐（但本项目无 FSM） |
| `-cm tgl` | 翻转覆盖率 | 推荐 |
| `-cm branch` | 分支覆盖率 | 推荐 |
| `-cm assert` | 断言覆盖率 | 可选 |
| `-cm_noconst` | 不追踪常量 | **必须加**（否则 URG 崩溃） |
| `-cm_seqnoconst` | sequence 中的常量不追踪 | 推荐 |
| `-cm_line contassign` | 行覆盖率包含连续赋值 | 推荐 |
| `-cm_tgl mda+structarr+fullintf` | 翻转覆盖包含多维数组/结构体 | 推荐 |
| `-cm_cond obs` | 条件覆盖率使用可观测语义 | 推荐 |

### 禁止的组合

- **不要**同时使用 `--code_cov` 和在模板中重复添加 `-cm` 标志（会导致标志重复）
- **不要**只仿真不加编译（`-so` 不能添加覆盖率，覆盖率是编译时选项）

<a id="urg-report"></a>
## 4. URG 报告生成

### 关键变更（v1.1）

VCS W-2024.09-SP1 工具已修复 URG 崩溃问题：
- `urg -dir vdb1 -dir vdb2 ...`（merge 多 vdb）**正常可用**
- 单 vdb 报告生成**正常可用**
- **不再需要"重试"或"用单一 vdb 代替 merge"** 等 workaround

### 标准命令

```bash
# 方式 1（推荐，与 v1.0 一致）：ksim 已自动合并为单一 simv.vdb，直接生成报告
urg -full64 \
    -dir /home/IC_verify/simdir/wujian100/test/logic_op.logic_op_smoke/simv.vdb \
    -report /path/to/output \
    -format text

# 方式 2（v1.1 新支持）：merge 多个 vdb 后再生成报告
urg -full64 \
    -dir /home/IC_verify/simdir/wujian100/test/logic_op.logic_op_smoke/simv.vdb \
    -dir /home/IC_verify/simdir/wujian100/test/logic_op.logic_op_and/simv.vdb \
    -dir /home/IC_verify/simdir/wujian100/test/logic_op.logic_op_or/simv.vdb \
    -report /path/to/output \
    -format text

# -format text 比默认 HTML 更稳定
# -full64 确保使用 64-bit 版本
```

### 仍然需要避免的错误做法

```bash
# ✗ 错误：用 build 目录下的 simv.vdb
urg -dir /path/to/build/logic_op/simv.vdb -report output  # 可能不包含测试数据

# ✗ 错误：不带 -full64
urg -dir ... -report ...  # 可能调用 32-bit 版本

# ✗ 错误：v1.0 时代的 workaround（已作废）
# urg -dir ... ; urg -dir ... ; urg -dir ...  # 不再需要重试循环
# rm -rf /path/to/output && urg ...           # 不再需要清理残留
```

<a id="coverage-analysis"></a>
## 5. 覆盖率数据解读

### DUT 覆盖率在哪里

**DUT 不会作为独立模块出现在 modlist 中！** DUT (`logic_op`) 实例化为 `dut`，其覆盖率数据在 **`tb_top` 层级**下。

查找 DUT 覆盖率：
```bash
grep -i "dut \|logic_op" cov_final/hierarchy.txt
```

输出示例：
```
SCORE  LINE   COND   TOGGLE FSM    BRANCH ASSERT
 74.68  97.06  88.46  19.87 --      93.33 --     dut              ← DUT 顶层
 70.52  95.00  85.00  13.19 --      88.89 --     u_logic_op_reg_ctrl  ← 子模块
```

### 分层解读

| 层级 | 含义 | 关注重点 |
|------|------|----------|
| `tb_top` (32.67%) | Testbench 顶层，含所有接口和连线 | 参考值，非验证目标 |
| `dut` (74.68%) | **DUT 实例，这是核心指标** | Line/Cond/Branch 应接近 100% |
| `u_logic_op_reg_ctrl` (70.52%) | DUT 子模块 | 同上 |
| `apb_env_pkg` (0%) | APB VIP 内部代码 | 不需要关注 |
| `uvm_pkg` (33.33%) | UVM 库代码 | 不需要关注 |
| 全局 SCORE (27.53%) | **全设计加权平均** | **误导性指标，不要关注** |

### 重点关注的指标

| 指标 | 目标 | 说明 |
|------|------|------|
| **DUT Line** | > 95% | 每一行 RTL 是否被执行过 |
| **DUT Condition** | > 90% | 每个条件表达式是否所有可能值都出现过 |
| **DUT Branch** | > 90% | 每个 if/else 分支是否都被执行过 |
| **DUT Toggle** | > 50% | 每个 bit 是否都翻转了 0→1 和 1→0 |
| **DUT FSM** | -- | 本项目无 FSM 覆盖 |
| 全局 SCORE | -- | **忽略！** 含 UVM/VIP 代码，稀释了 DUT 指标 |

### Toggle 覆盖率偏低的原因

4-bit 数据通路（`in1[3:0]`, `in2[3:0]`, `out[3:0]`）中，不是所有 bit 在所有测试中都有完整的 0→1 和 1→0 翻转。例如：
- 某 bit 在 AND 模式下始终为 0
- 某 bit 在 OR 模式下始终为 1
- 测试向量未覆盖比特级的全部翻转组合

Toggle 19.87% 对 4-bit 设计属于正常范围——如需提高，需要专门编写 toggle 定向测试。

<a id="troubleshooting"></a>
## 6. 常见问题与解决方案

| 问题 | 根因 | 解决方案 |
|------|------|----------|
| ~~URG segfault (merge 多 vdb)~~ | ~~VCS W-2024.09-SP1 已知 bug~~ | **v1.1 已修复** — VCS 工具已升级，多 vdb merge 正常 |
| ~~URG segfault (单 vdb)~~ | ~~旧报告目录残留~~ | **v1.1 已修复** — VCS 工具已升级，单 vdb 报告正常 |
| DUT 不在 modlist 中 | DUT 覆盖率在 tb_top 层级下 | `grep dut hierarchy.txt` |
| 全局 SCORE 很低 | UVM/VIP 代码占比大但覆盖率低 | **忽略全局 SCORE，看 DUT 指标** |
| `--code_cov` 标志未生效 | Mako 模板未使用 `MINUS__COMP` 变量 | 在 `vcs_<module>.file` 中显式添加 `-cm` 标志 |
| `ksim --code_cov -so` 无效 | 覆盖率是**编译时**选项 | 必须全量 `-co && -sim` 或不带 `-co/-so` |

<a id="checklist"></a>
## 7. 自检清单

### 编译前

```bash
# 1. 确认 vcs_<module>.file 包含 -cm 标志
grep "^-cm" dv/simulation/verif_env/ip/<module>/build/vcs_<module>.file

# 2. 确认环境变量
echo $KSIM_DB_DIR  # 应为 `pwd`
echo $PYTHONPATH   # pylib 在最前面
```

### 编译后

```bash
# 1. 确认 simv.vdb 存在
ls /home/IC_verify/simdir/wujian100/build/<module>/simv.vdb/

# 2. 确认所有测试完成
for t in <test_list>; do
  grep "UVM_CASE_PASS\|UVM_CASE_FAIL" /home/IC_verify/simdir/wujian100/test/<module>.$t/*.log
done
```

### 报告生成后

```bash
# 1. 确认报告文件存在
ls cov_final/dashboard.txt cov_final/hierarchy.txt

# 2. DUT 覆盖率指标是否合理
grep "dut " cov_final/hierarchy.txt

# 3. 确认所有测试都已包含
grep "Total tests" cov_final/dashboard.txt  # 应显示 Number of tests: 13 (12 + test)
```

---

## 快速参考命令

```bash
# === 环境设置 ===
cd /home/IC_verify/project/wujian100/dv && source dv.bashrc
source /home/IC_verify/project/wujian100/.claude/skills/ksim/scripts/sourceme.bash
export PYTHONPATH="/home/IC_verify/project/wujian100/.claude/skills/ksim/scripts/mypylib/pylib:/home/IC_verify/project/wujian100/.claude/skills/ksim/scripts/mypylib/lib:/home/IC_verify/project/wujian100/.claude/skills/ksim/scripts"
export KSIM_DB_DIR=/home/IC_verify

# === 覆盖率回归（v1.1：与 v1.0 命令完全一致，URG 工具已修复） ===
ksim -t logic_op::logic_op.py --code_cov --clean

# === 生成报告（直接运行，无需重试） ===
urg -full64 -dir <simv.vdb> -report <cov_final_dir> -format text

# === 查看 DUT 覆盖率 ===
grep "dut " cov_final/hierarchy.txt
grep "u_logic_op" cov_final/hierarchy.txt
```

---

## 版本历史

| 版本 | 日期 | 更新内容 |
|------|------|----------|
| v1.0 | 2026-05-06 | 初版，记录 VCS W-2024.09-SP1 URG 崩溃现象与 workaround |
| v1.1 | 2026-06-16 | VCS 工具层修复 URG segfault；删除"重试循环"、"rm -rf 输出"、"用单一 vdb 替代 merge"等过时 workaround；新增多 vdb merge 标准命令 |
