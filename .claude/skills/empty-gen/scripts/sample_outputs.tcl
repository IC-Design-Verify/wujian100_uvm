################################################################################
# sample_outputs.tcl
#
# Sample signal values under a DUT scope at multiple FSDB timestamps, to
# identify stable reset-state default values for use in a blackbox stub.
#
# This script is DUT-agnostic. It supports two ways to specify which signals
# to sample:
#
#   1. OUT_PORTS_FILE (preferred)  - path to a text file, one signal name
#      per line. Lines starting with '#' are comments. Whitespace is trimmed.
#
#   2. AUTO_DISCOVER=1             - recursively walk DUT_SCOPE and sample
#      every leaf scope (signal). Use when no explicit port list is known.
#
# Required env vars:
#   FSDB_PATH      - absolute path to the FSDB file
#   DUT_SCOPE      - full hierarchical scope of the DUT instance
#
# Optional env vars:
#   OUT_FILE        - output file path (default: /tmp/<dut_leaf>_output_values.txt)
#   OUT_PORTS_FILE  - one-port-per-line file with the signal names to sample
#   AUTO_DISCOVER   - "1" to auto-discover signals from DUT_SCOPE (fallback)
#   SAMPLE_TIMES_PS - whitespace-separated list of picosecond timestamps
#                     (default: "200000 1000000 10000000 50000000")
#
# Output format (whitespace-separated, easy to parse with awk/python):
#   # Output port values for <scope>
#   # Format: port<TAB>t1<TAB>t2<TAB>t3<TAB>t4<TAB>t5 (X = unresolved)
#   Port    200ns   1000ns  10000ns 50000ns 140075ns
#   sig1    X       X       0       0       6000fff8
#   ...
#
# Usage:
#   export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
#   export FSDB_PATH=<path_to_fsdb>
#   export DUT_SCOPE=<hierarchical_path>
#   export OUT_PORTS_FILE=<port_list>      # or AUTO_DISCOVER=1
#   $VERDI_HOME/bin/novas -play sample_outputs.tcl -batch
################################################################################

# ---- Required: FSDB_PATH, DUT_SCOPE ------------------------------------------
if {![info exists ::env(FSDB_PATH)] || $::env(FSDB_PATH) == ""} {
  puts "ERROR: FSDB_PATH env var is required"
  debExit
}
if {![info exists ::env(DUT_SCOPE)] || $::env(DUT_SCOPE) == ""} {
  puts "ERROR: DUT_SCOPE env var is required"
  debExit
}
# -----------------------------------------------------------------------------

# ---- Optional defaults -------------------------------------------------------
if {![info exists ::env(OUT_FILE)]} {
  # Use the last segment of DUT_SCOPE as the prefix
  set scope $::env(DUT_SCOPE)
  set parts [split $scope "."]
  set leaf [lindex $parts end]
  set ::env(OUT_FILE) "/tmp/${leaf}_output_values.txt"
}
if {![info exists ::env(SAMPLE_TIMES_PS)]} {
  # Default sample points in picoseconds (200ns, 1us, 10us, 50us).
  # The end-of-sim time is appended below.
  set base_times {200000 1000000 10000000 50000000}
} else {
  set base_times $::env(SAMPLE_TIMES_PS)
}
# -----------------------------------------------------------------------------

viaSetupL1Apps

set fsdb_hdl [npi_fsdb_open -name $::env(FSDB_PATH)]
if {$fsdb_hdl == ""} {
  puts "ERROR: Cannot open FSDB: $::env(FSDB_PATH)"
  debExit
}

# Get FSDB time range and build final sample-time list (append end-of-sim).
set min_t [npi_fsdb_min_time -file $fsdb_hdl]
set max_t [npi_fsdb_max_time -file $fsdb_hdl]
set sample_times $base_times
lappend sample_times $max_t

puts "FSDB: $::env(FSDB_PATH)"
puts "Time range: $min_t - $max_t ps"
puts "DUT scope: $::env(DUT_SCOPE)"
puts "Out file:  $::env(OUT_FILE)"

# Get DUT scope handle
set scope_hdl [npi_fsdb_scope_by_name -file $fsdb_hdl -name $::env(DUT_SCOPE) -scope ""]
if {$scope_hdl == ""} {
  puts "ERROR: Cannot find scope $::env(DUT_SCOPE)"
  puts "Run find_dut_scope.tcl first to locate the correct scope"
  npi_fsdb_close -file $fsdb_hdl
  debExit
}

# ---- Resolve the signal list ------------------------------------------------
# Source: OUT_PORTS_FILE if provided, otherwise AUTO_DISCOVER.
proc is_leaf {scope} {
  # A leaf scope (signal) has no child scopes.
  set it [npi_fsdb_iter_child_scope -scope $scope]
  if {$it == ""} { return 1 }
  set c [npi_fsdb_iter_scope_next -iter $it]
  npi_fsdb_iter_scope_stop -iter $it
  return [expr {$c == ""}]
}

proc collect_signals_recursive {scope out_list} {
  # Recursively walk a scope and append every leaf-scope name to out_list.
  upvar $out_list L
  if {$scope == ""} { return }
  set it [npi_fsdb_iter_child_scope -scope $scope]
  if {$it == ""} { return }
  set child [npi_fsdb_iter_scope_next -iter $it]
  while {$child != ""} {
    if {[is_leaf $child]} {
      set name [npi_fsdb_scope_property_str -scope $child -type npiFsdbScopeName]
      lappend L $name
    } else {
      collect_signals_recursive $child L
    }
    set child [npi_fsdb_iter_scope_next -iter $it]
  }
  npi_fsdb_iter_scope_stop -iter $it
}

set output_ports [list]
if {[info exists ::env(OUT_PORTS_FILE)] && $::env(OUT_PORTS_FILE) != ""} {
  set pf $::env(OUT_PORTS_FILE)
  if {![file exists $pf]} {
    puts "ERROR: OUT_PORTS_FILE does not exist: $pf"
    npi_fsdb_close -file $fsdb_hdl
    debExit
  }
  set fh [open $pf r]
  while {[gets $fh line] >= 0} {
    set t [string trim $line]
    if {$t == ""} { continue }
    if {[string index $t 0] == "#"} { continue }
    lappend output_ports $t
  }
  close $fh
  puts "Source:    $pf ([llength $output_ports] ports)"
} elseif {[info exists ::env(AUTO_DISCOVER)] && $::env(AUTO_DISCOVER) == "1"} {
  collect_signals_recursive $scope_hdl output_ports
  puts "Source:    AUTO_DISCOVER ([llength $output_ports] signals)"
} else {
  puts "ERROR: Set OUT_PORTS_FILE or AUTO_DISCOVER=1 to specify signals"
  npi_fsdb_close -file $fsdb_hdl
  debExit
}
# -----------------------------------------------------------------------------

# Build column header (timestamps in ns, for human readability)
set header "Port"
foreach t $sample_times { append header "\t[expr {$t/1000}]ns" }

# Open output file
set out_file [open $::env(OUT_FILE) "w"]
puts $out_file "# Output port values for $::env(DUT_SCOPE)"
puts $out_file "# Format: port<TAB>t1<TAB>t2<TAB>t3<TAB>t4<TAB>t5 (X = unresolved)"
puts $out_file $header

foreach port $output_ports {
  set sig_hdl [npi_fsdb_sig_by_name -file $fsdb_hdl -name $port -scope $scope_hdl]
  if {$sig_hdl == ""} {
    set line "$port\tMISSING"
    for {set i 0} {$i < [llength $sample_times]} {incr i} { append line "\t-" }
    puts $out_file $line
    continue
  }

  set vct_hdl [npi_fsdb_create_vct -sig $sig_hdl]
  if {$vct_hdl == ""} {
    set line "$port\tVCT_FAIL"
    for {set i 0} {$i < [llength $sample_times]} {incr i} { append line "\t-" }
    puts $out_file $line
    continue
  }

  set line $port
  foreach t $sample_times {
    npi_fsdb_goto_time -vct $vct_hdl -time $t
    set hex_val [npi_fsdb_vct_value -vct $vct_hdl -format npiFsdbHexStrVal]
    append line "\t$hex_val"
  }
  puts $out_file $line

  npi_fsdb_release_vct -vct $vct_hdl
}

close $out_file
puts "Wrote: $::env(OUT_FILE)"

npi_fsdb_close -file $fsdb_hdl
debExit