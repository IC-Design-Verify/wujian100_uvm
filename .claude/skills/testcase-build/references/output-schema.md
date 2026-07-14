# Stage 3 Output Schema

## testname.f Format

One test class name per line. Lines starting with `//` or `#` are comments.

```
# P0 tests
basic_tx_test
basic_rx_test
reset_test

# P1 tests
config_all_modes_test
parity_error_test
framing_error_test

# P2 tests
back_to_back_stress_test
boundary_value_test
```

## testplan.json Updates

When Stage 3 generates a test for a scenario, it updates these fields in `testplan.json`:

| Field | Before | After |
|-------|--------|-------|
| `status` | `"not_covered"` | `"covered"` |
| `covered_by` | `null` | `"<test_class_name>"` |
| `vseq` | `null` | `"<vseq_class_name>"` |
| `source_files` | `[]` | `["uvm_tb/<project>_tests.sv", "uvm_tb/<project>_test_vseqs.sv"]` |

## Batch Status Output

After each priority batch, print:

```
=== Stage 3 Batch Status ===
Priority: P0
Tests generated: 5
Tests registered in testname.f: 5
Package includes updated: Yes
Ready for Stage 5 compilation: Yes
```
