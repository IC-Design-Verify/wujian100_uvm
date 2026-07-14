################################################################################
# find_dut_scope.tcl
#
# Recursively search an FSDB for instances whose module definition name
# matches a given pattern. Prints all matching hierarchical paths.
#
# Required env vars (override the defaults via env or edit defaults below):
#   DUT_PATTERN  - substring to match against module def name (case-insensitive)
#   FSDB_PATH    - absolute path to the FSDB file
#
# Usage:
#   export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
#   export DUT_PATTERN=<module_substring>
#   export FSDB_PATH=<path_to_fsdb>
#   $VERDI_HOME/bin/novas -play find_dut_scope.tcl -batch
################################################################################

# ---- Configurable defaults ---------------------------------------------------
# Override via env vars (DUT_PATTERN, FSDB_PATH).
# The defaults below are placeholders; in normal use they must be supplied
# by the caller. They are only used when the env vars are not set, so this
# script never hardcodes a specific project or DUT.
if {![info exists ::env(DUT_PATTERN)]} { set ::env(DUT_PATTERN) "DUT" }
if {![info exists ::env(FSDB_PATH)]} {
  puts "ERROR: FSDB_PATH env var is required"
  debExit
}
# ---------------------------------------------------------------------------

viaSetupL1Apps

set fsdb_hdl [npi_fsdb_open -name $::env(FSDB_PATH)]
if {$fsdb_hdl == ""} {
  puts "ERROR: Cannot open FSDB: $::env(FSDB_PATH)"
  debExit
}

# Get top scope
set scope_iter [npi_fsdb_iter_top_scope -file $fsdb_hdl]
set top_scope [npi_fsdb_iter_scope_next -iter $scope_iter]
if {$scope_iter != ""} { npi_fsdb_iter_scope_stop -iter $scope_iter }
if {$top_scope == ""} {
  puts "ERROR: No top scope found in FSDB"
  npi_fsdb_close -file $fsdb_hdl
  debExit
}

set top_name [npi_fsdb_scope_property_str -scope $top_scope -type npiFsdbScopeFullName]
puts "Top scope: $top_name"
puts "Pattern:   $::env(DUT_PATTERN)"
puts "Searching..."
puts "---"

# Recursive search: traverse every scope, print matches.
proc find_dut {scope fullpath pattern} {
  if {$scope == ""} { return }
  set scope_name [npi_fsdb_scope_property_str -scope $scope -type npiFsdbScopeName]
  set def_name   [npi_fsdb_scope_property_str -scope $scope -type npiFsdbScopeDefName]
  set new_path   "${fullpath}.${scope_name}"

  if {[string match -nocase "*${pattern}*" $def_name]} {
    puts "FOUND: $new_path (def=$def_name)"
  }

  set child_iter [npi_fsdb_iter_child_scope -scope $scope]
  if {$child_iter != ""} {
    set child [npi_fsdb_iter_scope_next -iter $child_iter]
    while {$child != ""} {
      find_dut $child $new_path $pattern
      set child [npi_fsdb_iter_scope_next -iter $child_iter]
    }
    npi_fsdb_iter_scope_stop -iter $child_iter
  }
}

find_dut $top_scope "" $::env(DUT_PATTERN)
puts "---"
puts "Done."

npi_fsdb_close -file $fsdb_hdl
debExit