# Tools Quick Reference

## DesignComprehension — Netlist Exploration (~31 apps)

| Tool | Function |
|------|----------|
| `getModHier` | Module hierarchy (CSV output supported) |
| `getModIO` | Module input/output ports |
| `getModSignal` | Internal signals of a module |
| `getModParam` | Module parameters |
| `getPortConnection` | Port connection relationships |
| `getConstAssign` | Constant-assigned signals |
| `getEqSignal` | Equivalent signals (same driver) |
| `getFanInRegister` / `getFanOutRegister` | Fan-in / fan-out registers |
| `getGenScope` | Generate block scopes |
| `findInst` | Instance search (wildcard supported) |
| `findInstDefWild` | Wildcard instance definition search |
| `getForceStmt` | Force statement locations |
| `listRegisters` | Register list |
| `listModDefine` | Module definition list |
| `listIncludeFiles` | Include file list |
| `listMDA` | MDA signal list |
| `listAllFilesInDesign` | All files in design |
| `listClassHier` | Class hierarchy |
| `listObjDef` | Object definitions |
| `dumpEnumTypedef` | Enum/typedef definitions |
| `detectFSMMacroDefine` | FSM macro detection |
| `detectSeqOrComb` | Combinational vs sequential logic detection |
| `getTypedefSize` | Typedef bit width |
| `getPGPinConnection` | PG pin connections |
| `getPowerDomainSupply` | Power domain / supply |
| `getNetlistOverview` | Netlist overview |
| `getInstFaninFanoutSig` | Instance fan-in/fan-out signals |
| `getSigFaninInst` | Signal fan-in instances |
| `signalSearch` | Signal search |
| `pyApp` | Python app wrapper |

## DesignValidation — Design Quality Checks (11 apps)

| Tool | Function |
|------|----------|
| `checkModRegIn` | Input reg check |
| `checkModRegOut` | Output reg check |
| `checkModPortConnModport` | Port connection check |
| `checkModPortDeclModport` | Port declaration check |
| `detectAssignBitWidthMismatch` | Bit width mismatch |
| `detectConstPort` | Constant port detection |
| `detectDanglingPort` | Dangling port detection |
| `detectOpenPort` | Unconnected port |
| `detectPortZ` | Z / HiZ port detection |
| `detectTriState` | Tri-state gate detection |
| `signalBusCheck` | Bus signal check |
| `moduleArrayInstantiationCheck` | Array instantiation check |
| `signalExistCheckDesign` | Signal exists (design) |
| `signalExistCheckFSDB` | Signal exists (FSDB) |

## DesignManipulation — Netlist Modification (7 apps)

| Tool | Function |
|------|----------|
| `renameInstance` | Instance renaming |
| `delete_pass_through` | Delete pass-through modules |
| `npiWriteDummyMod` | Write dummy module (bbox) |
| `simex` | Write dummy module (bbox, alternate impl) |
| `discon` | Disconnect connections |
| `traceInstConnection` | Trace instance connections |
| `traceMemOnChain` | Trace memory chain |

## FsdbInvestigation — FSDB Waveform Analysis (~14 apps + C++)

| Tool | Function |
|------|----------|
| `npiFsdbManip` | General FSDB operations |
| `npiFsdbFreqDuty` | Clock frequency / duty cycle |
| `npiFsdbMinPulse` | Minimum pulse detection |
| `npiFsdbReportClkFreq` | Clock frequency report |
| `npiFsdbReportForce` | Force statement report |
| `npiFsdbReportMDA` | MDA signal report |
| `npiFsdbReportVCX` | VCX report |
| `npiFsdbReportSimState2CSOC` | Simulation state report |
| `npiWaveExtractScopeSig` | Extract scope signals |
| `npiWaveNinja` | Waveform analysis |
| `npiWaveSigStates` | Signal state analysis |
| `npiWaveSigToggle` | Signal toggle rate |
| `npiWaveT` | Waveform time operations |
| `npi_digital_fsdb_cmp` | Digital FSDB comparison |
| `npi_digital_fsdb_clk_freq_cmp` | Clock frequency comparison |
| `npi_analog_fsdb_cmp` | Analog FSDB comparison |
| `npi_collect_exclude_mod` | Collect excluded modules |
| `npi_expand_wildcard_delay` | Expand wildcard delays |
| `npi_expand_wildcard_map` | Expand wildcard mappings |
| `serialSig2Parallel` | Serial-to-parallel signal conversion |
| `wi_merge_fsdb` | Merge FSDB files |

## Coverage

| Tool | Function |
|------|----------|
| `decovan` | Coverage analysis (batch TCL/Python) |
| `GenTglTestTable` | Toggle test table generator (C++, needs $VCS_HOME) |
