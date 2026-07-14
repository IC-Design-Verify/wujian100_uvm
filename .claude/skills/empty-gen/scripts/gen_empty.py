#!/usr/bin/env python3
"""
gen_empty.py - Generate a minimal blackbox .empty.v module from sampled FSDB values.

Input:  A values file (tab-separated: port, t1, t2, t3, t4, t5)
        and a port-list file (one port per line, in module header order)
Output: A SystemVerilog module with:
        - Verbatim port list (from the port-list file)
        - Input/output port declarations
        - assign <output> = <default_value>; for each output port
        - No internal wires, no behavioral logic

Usage:
    python3 gen_empty.py \\
        --module <dut_module_name> \\
        --ports <dut>_ports.txt \\
        --values <dut>_output_values.txt \\
        --output <dut>.empty.v

Port-list file format (one per line, in module header order):
    port_a,
    port_b,
    ...

Values file format (tab-separated, from sample_outputs.tcl):
    port_a   XXXXXXXX   XXXXXXXX   00000000   00000000   6000fff8
    ...

The "default value" is the value at the column corresponding to the stable
reset state. The script picks the first non-X value, scanning left-to-right.
"""
import argparse
import re
import sys
from pathlib import Path


def parse_values_file(values_path: Path) -> dict:
    """Parse whitespace-separated values file. Returns {port_name: default_hex_string}.

    Accepts tab- or space-separated columns (Tcl's `puts` emits literal `\t` but
    some tools normalize to spaces). Skips the header line if it contains 'ns' or 'us'.

    Picks the first column whose value does not contain 'x' or 'X'.
    """
    defaults = {}
    for line in values_path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        # Split on any whitespace (handles both tabs and spaces)
        parts = line.split()
        if len(parts) < 2:
            continue
        port = parts[0]
        # Skip header row: identified by having time-unit labels in value columns
        # (e.g., "200ns", "1us"). A real data row has only hex digits / 'X' / '-'.
        if any(v.lower().endswith(("ns", "us")) for v in parts[1:]):
            continue
        # Find first non-X value
        chosen = "0"
        for v in parts[1:]:
            if v and v not in ("-", "MISSING", "VCT_FAIL") and "x" not in v.lower():
                chosen = v
                break
        defaults[port] = chosen
    return defaults


def parse_ports_file(ports_path: Path) -> list:
    """Parse a port-list file. Returns list of port names (without trailing comma)."""
    ports = []
    for line in ports_path.read_text().splitlines():
        line = line.strip().rstrip(",").strip()
        if line and not line.startswith("#"):
            ports.append(line)
    return ports


def extract_module_ports(module_path: Path):
    """Extract port list, input port declarations, and output port declarations
    from an existing module .v file (e.g. the full RTL or existing .empty.v).

    Returns: (port_list_lines, input_decls, output_decls)
    where input_decls and output_decls are lists of (direction, width, name) tuples.
    """
    text = module_path.read_text()
    # Find module header (between "module <name>(" and ");")
    m = re.search(r"module\s+\w+\s*\((.*?)\)\s*;", text, re.DOTALL)
    if not m:
        raise ValueError(f"Cannot find module header in {module_path}")
    port_block = m.group(1)
    ports = [p.strip().rstrip(",") for p in port_block.split("\n") if p.strip() and p.strip() != ","]

    # Parse port declarations (input/output lines after the header)
    inout_decls = []
    for line in text.splitlines():
        line = line.strip()
        m = re.match(r"^(input|output)\s*(\[[^\]]+\])?\s*(\w+)\s*;", line)
        if m:
            direction = m.group(1)
            width = m.group(2) or "[0:0]" if False else (m.group(2) or "")
            name = m.group(3)
            inout_decls.append((direction, width, name))

    return ports, inout_decls


def width_to_int(width_str: str) -> int:
    """Convert '[31:0]' or '[5:0]' to integer bit width. Returns 1 for scalar."""
    if not width_str:
        return 1
    m = re.match(r"\[(\d+)\s*:\s*(\d+)\]", width_str)
    if not m:
        return 1
    hi, lo = int(m.group(1)), int(m.group(2))
    return abs(hi - lo) + 1


def format_value_hex(hex_str: str, width: int) -> str:
    """Format hex value with underscores for readability and proper bit-width literal.

    e.g. '30000000' with width 32 -> '32'h3000_0000'
         '3'        with width 2  -> '2'h3'
         '1'        with width 1  -> '1'h1'
         '0'        with width 32 -> '32'h0'
    """
    # Strip leading zeros for cleaner display, keep at least 1 digit
    clean = hex_str.lstrip("0") or "0"
    # Insert underscore every 4 chars from the right
    formatted = ""
    while len(clean) > 4:
        formatted = "_" + clean[-4:] + formatted
        clean = clean[:-4]
    formatted = clean + formatted
    return f"{width}'h{formatted}"


def render_module(
    module_name: str,
    port_list: list,
    inout_decls: list,
    defaults: dict,
) -> str:
    """Render the .empty.v file as a string."""
    lines = []
    lines.append("/*Copyright 2020-2021 T-Head Semiconductor Co., Ltd.")
    lines.append("")
    lines.append("Licensed under the Apache License, Version 2.0 (the \"License\");")
    lines.append("you may not use this file except in compliance with the License.")
    lines.append("You may obtain a copy of the License at")
    lines.append("")
    lines.append("    http://www.apache.org/licenses/LICENSE-2.0")
    lines.append("")
    lines.append("Unless required by applicable law or agreed to in writing, software")
    lines.append("distributed under the License is distributed on an \"AS IS\" BASIS,")
    lines.append("WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.")
    lines.append("See the License for the specific language governing permissions and")
    lines.append("limitations under the License.")
    lines.append("*/")
    lines.append("")
    lines.append("// &ModuleBeg; @33")
    lines.append(f"module {module_name}(")
    for p in port_list:
        lines.append(f"  {p},")
    lines.append(");")
    lines.append("")
    lines.append("//&Ports;")
    lines.append("")
    lines.append("// =====================================================")
    lines.append("// Output port default assignments only.")
    lines.append(f"// Default values sampled from FSDB waveform (ksim --w=fsdb).")
    lines.append("// =====================================================")
    lines.append("")

    # Output port declarations
    for direction, width, name in inout_decls:
        if direction == "output":
            decl = f"{direction}  {width + '  ' if width else '         '}{name};"
            lines.append(decl)
    lines.append("")

    # Input port declarations
    for direction, width, name in inout_decls:
        if direction == "input":
            decl = f"{direction}  {width + '  ' if width else '         '}{name};"
            lines.append(decl)
    lines.append("")

    lines.append("// =====================================================")
    lines.append("// Output port default-value assignments")
    lines.append("// (sampled from FSDB; leftmost non-X value is the reset default)")
    lines.append("// =====================================================")
    for direction, width, name in inout_decls:
        if direction == "output":
            hex_val = defaults.get(name, "0")
            w = width_to_int(width)
            val_lit = format_value_hex(hex_val, w)
            # Pad name to 28 chars for alignment
            lines.append(f"assign  {name.ljust(28)}= {val_lit};")
    lines.append("")
    lines.append("// &ModuleEnd; @191")
    lines.append("endmodule")
    lines.append("")
    return "\n".join(lines)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--module", required=True, help="Module name (e.g. <dut_module_name>)")
    p.add_argument("--source", type=Path, help="Source .v file to extract port list/decls (full RTL or existing empty)")
    p.add_argument("--ports", type=Path, help="Plain-text port list (alternative to --source)")
    p.add_argument("--values", required=True, type=Path, help="Sampled values file (from sample_outputs.tcl)")
    p.add_argument("--output", required=True, type=Path, help="Output .empty.v file path")
    args = p.parse_args()

    # Get port list and declarations
    if args.source:
        port_list, inout_decls = extract_module_ports(args.source)
    elif args.ports:
        port_list = parse_ports_file(args.ports)
        # Without source, can't get widths; assume 1-bit for outputs
        inout_decls = [("output", "", p) for p in port_list]
    else:
        p.error("Must specify either --source or --ports")

    # Get sampled defaults
    defaults = parse_values_file(args.values)

    # Render
    content = render_module(args.module, port_list, inout_decls, defaults)

    # Write
    args.output.write_text(content)
    print(f"Generated: {args.output} ({len(content)} bytes, {len(inout_decls)} ports, {sum(1 for d,*_ in inout_decls if d=='output')} output assignments)")


if __name__ == "__main__":
    main()
