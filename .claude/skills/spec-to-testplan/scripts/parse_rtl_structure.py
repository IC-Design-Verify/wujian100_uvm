#!/usr/bin/env python3
"""Extract structural information from SystemVerilog RTL files.

Generic parser — works with any SystemVerilog design.
Uses regex-based extraction for standard constructs.
Output is structured JSON for downstream agent analysis.

Usage:
    python3 parse_rtl_structure.py <rtl_file> [<rtl_file2> ...]
    python3 parse_rtl_structure.py --dir <rtl_directory>

Output (stdout): JSON with modules, ports, parameters, macros, FSM candidates.
"""

import json
import os
import re
import sys


def read_file(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def strip_comments(text):
    """Remove single-line and multi-line comments."""
    text = re.sub(r"//.*?$", "", text, flags=re.MULTILINE)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return text


def extract_modules(text):
    """Extract module declarations with port lists."""
    modules = []
    # Match: module name (...); or module name #(...) (...);
    mod_pattern = re.compile(
        r"module\s+(\w+)\s*(?:#\s*\([^)]*\))?\s*\(([^;]*?)\)\s*;",
        re.DOTALL,
    )
    for m in mod_pattern.finditer(text):
        mod_name = m.group(1)
        port_block = m.group(2)
        ports = extract_ports(port_block)
        modules.append({"name": mod_name, "ports": ports})
    return modules


def extract_ports(port_text):
    """Parse port declarations from module header."""
    ports = []
    # Match: input/output/inout [logic/wire/reg] [signed] [width] name [, name]
    port_pattern = re.compile(
        r"(input|output|inout)\s+"
        r"(?:(logic|wire|reg)\s+)?"
        r"(?:(signed)\s+)?"
        r"(?:\[([^\]]+)\]\s*)?"
        r"(\w+)",
        re.IGNORECASE,
    )
    for p in port_pattern.finditer(port_text):
        direction = p.group(1).lower()
        width_expr = p.group(4)
        name = p.group(5)

        width = 1
        width_str = ""
        if width_expr:
            width_str = width_expr.strip()
            # Try to evaluate simple widths like "31:0" -> 32
            range_match = re.match(r"(\d+)\s*:\s*(\d+)", width_expr)
            if range_match:
                hi = int(range_match.group(1))
                lo = int(range_match.group(2))
                width = abs(hi - lo) + 1
            else:
                width = -1  # complex expression, agent must resolve

        ports.append({
            "name": name,
            "direction": direction,
            "width": width,
            "width_expr": width_str,
        })
    return ports


def extract_parameters(text):
    """Extract parameter and localparam declarations."""
    params = []
    param_pattern = re.compile(
        r"(parameter|localparam)\s+(?:(?:integer|real|logic|bit)\s+)?"
        r"(?:\[[^\]]+\]\s*)?"
        r"(\w+)\s*=\s*([^;,\n]+)",
        re.IGNORECASE,
    )
    for p in param_pattern.finditer(text):
        kind = p.group(1).lower()
        name = p.group(2)
        value = p.group(3).strip().rstrip(",")
        params.append({"kind": kind, "name": name, "value": value})
    return params


def extract_defines(text_with_comments):
    """Extract `define macros (must run on original text, before comment strip)."""
    defines = []
    define_pattern = re.compile(
        r"`define\s+(\w+)\s+(.*?)$", re.MULTILINE
    )
    for d in define_pattern.finditer(text_with_comments):
        name = d.group(1)
        value = d.group(2).strip()
        defines.append({"name": name, "value": value})
    return defines


def extract_fsm_candidates(text):
    """Heuristic: find state registers used in case blocks anywhere in the file."""
    candidates = []
    # Strategy: find ALL case/casez/casex statements in the file,
    # regardless of whether they're in always blocks with begin/end.
    # This catches: always @(*) case(x), always @(*) begin case(x) end, etc.
    case_pattern = re.compile(r"case[zx]?\s*\((\w+)\)", re.IGNORECASE)

    for cm in case_pattern.finditer(text):
        state_var = cm.group(1)
        # Already found this variable?
        if state_var in [c["state_register"] for c in candidates]:
            continue

        # Find the case block body: from this case to matching endcase
        start = cm.end()
        # Simple endcase search (not nested-aware, but good enough for FSM detection)
        endcase_match = re.search(r"\bendcase\b", text[start:])
        if not endcase_match:
            continue
        case_body = text[start:start + endcase_match.start()]

        # Extract case labels (lines starting with identifier followed by colon)
        case_labels = re.findall(r"^\s*(\w+)\s*:", case_body, re.MULTILINE)
        # Filter out common non-state labels
        states = [
            s for s in case_labels
            if s.lower() not in ("default", "begin", "end")
        ]
        if len(states) >= 2:  # at least 2 states to be a real FSM
            candidates.append({
                "state_register": state_var,
                "potential_states": states[:30],
                "state_count": len(states),
            })
    return candidates


def extract_registers(text):
    """Extract reg/logic variable declarations (not ports)."""
    regs = []
    # Match standalone reg/logic declarations (not in port list)
    reg_pattern = re.compile(
        r"^\s*(?:reg|logic)\s+(?:\[[^\]]+\]\s*)?(\w+)\s*(?:\[[^\]]*\])?\s*[;=]",
        re.MULTILINE,
    )
    for r in reg_pattern.finditer(text):
        name = r.group(1)
        regs.append(name)
    return list(set(regs))


def extract_instantiations(text):
    """Extract module instantiations."""
    insts = []
    # Match: module_name instance_name (...)
    # Heuristic: identifier followed by identifier followed by (
    inst_pattern = re.compile(
        r"^\s*(\w+)\s+(?:#\s*\([^)]*\)\s*)?(\w+)\s*\(",
        re.MULTILINE,
    )
    # Filter out known keywords that look like instantiations
    keywords = {
        "module", "endmodule", "function", "endfunction", "task", "endtask",
        "always", "initial", "assign", "if", "else", "for", "while",
        "case", "casez", "casex", "begin", "end", "generate",
        "input", "output", "inout", "wire", "reg", "logic", "integer",
        "parameter", "localparam", "typedef", "enum", "struct",
    }
    for m in inst_pattern.finditer(text):
        mod_type = m.group(1)
        inst_name = m.group(2)
        if mod_type.lower() not in keywords and inst_name.lower() not in keywords:
            insts.append({"module_type": mod_type, "instance_name": inst_name})
    return insts


def parse_file(filepath):
    """Parse a single RTL file and return structured data."""
    raw_text = read_file(filepath)
    clean_text = strip_comments(raw_text)

    return {
        "file": filepath,
        "modules": extract_modules(clean_text),
        "parameters": extract_parameters(clean_text),
        "defines": extract_defines(raw_text),
        "fsm_candidates": extract_fsm_candidates(clean_text),
        "registers": extract_registers(clean_text),
        "instantiations": extract_instantiations(clean_text),
    }


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <file.sv> [<file2.sv> ...]", file=sys.stderr)
        print(f"       {sys.argv[0]} --dir <rtl_directory>", file=sys.stderr)
        sys.exit(1)

    files = []
    if sys.argv[1] == "--dir":
        if len(sys.argv) < 3:
            print("Error: --dir requires a directory path", file=sys.stderr)
            sys.exit(1)
        rtl_dir = sys.argv[2]
        for f in sorted(os.listdir(rtl_dir)):
            if f.endswith((".sv", ".v", ".svh", ".vh")):
                files.append(os.path.join(rtl_dir, f))
    else:
        files = sys.argv[1:]

    results = []
    for f in files:
        if not os.path.isfile(f):
            print(f"Warning: {f} not found, skipping", file=sys.stderr)
            continue
        results.append(parse_file(f))

    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
