#!/usr/bin/env python3
"""Field-level audit: userguide.txt (spec) vs module_analysis docs, plus RTL port audit."""
import re, sys, json

ROOT = "/home/IC_verify/project/wujian100_uvm"
UG = open(f"{ROOT}/doc_summary/module_analysis/_src/userguide.txt").read().split("\n")

CH = {
 "tim": (399,518), "dma": (519,695), "usi": (696,1114), "wdt": (1115,1190),
 "pwm": (1191,1858), "rtc": (1859,1955), "gpio": (1956,2094),
}

def norm_bits(b):
    b = b.replace(" ", "").strip("`")
    b = b.replace("[","").replace("]","")
    m = re.match(r"^(\d+):(\d+)$", b)
    if m: return f"{int(m.group(1))}:{int(m.group(2))}"
    if re.match(r"^\d+$", b): return str(int(b))
    return b

def norm_name(n):
    n = re.sub(r"\[[^\]]*\]$", "", n.strip())  # drop trailing bit-range suffix e.g. RPL[2:0]
    return re.sub(r"[\s_`]+", "", n).lower()

RESERVED_ALIAS = {"", "n/a", "reserved", "reserved,readaszero", "reserved,readaszero(0).", "-"}

def names_match(a, b):
    na, nb = norm_name(a), norm_name(b)
    if na == nb: return True
    if not na or not nb: return True  # one side unparseable -> not a name defect
    if na in RESERVED_ALIAS and nb in RESERVED_ALIAS: return True
    if "reserved" in na and "reserved" in nb: return True
    if na.startswith(nb) or nb.startswith(na): return True
    return False

def norm_acc(a):
    a = a.strip().upper().replace(" ", "")
    m = {"READ/WRITE":"R/W","READ":"R","WRITE":"W","R/W":"R/W","RW":"R/W","R":"R","W":"W","RO":"RO","WO":"WO","W1C":"W1C","RC":"R"}
    return m.get(a, "")  # unknown/garbage -> skip comparison

def norm_rst(v):
    v = v.strip().replace("’","'").replace("`","'").replace("_","")
    # earliest match wins: hex (0x..) and verilog-literal (N'b/N'h) candidates by position
    cands = []
    for m in re.finditer(r"0[xX]([0-9a-fA-F]+)", v): cands.append((m.start(), int(m.group(1),16)))
    for m in re.finditer(r"(\d+)'b([01xXzZ]+)", v):
        try: cands.append((m.start(), int(m.group(2).replace("x","0").replace("z","0"),2)))
        except: pass
    for m in re.finditer(r"(\d+)'h([0-9a-fA-F]+)", v): cands.append((m.start(), int(m.group(2),16)))
    if cands: return sorted(cands)[0][1]
    if re.match(r"^\d+$", v): return int(v)
    return None

# ---------- parse user guide chapter ----------
def parse_ug(a, b):
    lines = UG[a-1:b]
    regs = []  # {name, offset, access, reset, width, fields:[{bits,name,acc,rst}]}
    # memory map tables: header contains "Address offset" or "offset"
    i = 0
    while i < len(lines):
        ln = lines[i]
        if ln == "[TABLE]":
            tbl = []
            j = i+1
            while j < len(lines) and lines[j] != "[/TABLE]":
                tbl.append(lines[j]); j += 1
            cells0 = [c.strip() for c in tbl[0].strip().strip("|").split("|")] if tbl else []
            hdr = "|".join(cells0).lower()
            if "address offset" in hdr or ("offset" in hdr and "name" in hdr):
                # memory map table
                idx = {c.lower(): k for k,c in enumerate(cells0)}
                for row in tbl[1:]:
                    cells = [c.strip() for c in row.strip().strip("|").split("|")]
                    if len(cells) < 2: continue
                    def col(*keys):
                        for k in keys:
                            for h,n in idx.items():
                                if k in h and n < len(cells): return cells[n]
                        return ""
                    name = col("name"); off = col("offset"); acc = col("access"); rst = col("reset"); w = col("width")
                    mo = re.search(r"0[xX][0-9a-fA-F]+", off)
                    if name and mo:
                        regs.append({"name": name, "offset": int(mo.group(0),16),
                                     "access": norm_acc(acc), "reset": norm_rst(rst), "width": w,
                                     "fields": [], "src": "map"})
            i = j
        i += 1
    # per-register field tables: prose "Address offset: 0x.." then next [TABLE]
    for i, ln in enumerate(lines):
        m = re.search(r"Address [Oo]ffset:\s*(0[xX][0-9a-fA-F]+)", ln)
        if not m: continue
        off = int(m.group(1),16)
        # register name: nearest previous prose line that isn't meta
        name = ""
        for j in range(i-1, max(0,i-6), -1):
            l = lines[j].strip()
            if not l or l in ("[TABLE]","[/TABLE]") or l.startswith("|"): continue
            if re.match(r"^(Name:|Table|Read/write|Description:|Size:)", l): continue
            if l.endswith(".") or l.startswith("Note"): continue  # description sentence
            name = re.sub(r"^\[\d+\]\s*", "", l); break
        # find next table
        fields = []
        for j in range(i+1, min(len(lines), i+8)):
            if lines[j] == "[TABLE]":
                k = j+1
                tbl=[]
                while k < len(lines) and lines[k] != "[/TABLE]":
                    tbl.append(lines[k]); k+=1
                if tbl:
                    hdr = [c.strip().lower() for c in tbl[0].strip().strip("|").split("|")]
                    if "bits" in hdr[0] or "field" in hdr[0]:
                        ni = next((k for k,h in enumerate(hdr) if h=="name"), None)
                        ai = next((k for k,h in enumerate(hdr) if h in ("r/w","access")), None)
                        ri = next((k for k,h in enumerate(hdr) if "default" in h), None)
                        for row in tbl[1:]:
                            cells=[c.strip() for c in row.strip().strip("|").split("|")]
                            if len(cells) < 2: continue
                            bits = norm_bits(cells[0])
                            desc = cells[-1]
                            if ni is not None and ni < len(cells):
                                fname = cells[ni]
                            elif len(cells) > len(hdr):
                                fname = cells[2]  # DMA style: | bits | R/W | NAME | desc |
                            else:
                                fname = ""
                            acc = norm_acc(cells[ai]) if ai is not None and ai < len(cells) else ""
                            rst = norm_rst(cells[ri]) if ri is not None and ri < len(cells) else None
                            if rst is None:
                                rst = norm_rst(desc.split("eset Value:")[-1].split("eset value:")[-1]) if "eset" in desc else None
                            fields.append({"bits":bits,"name":fname,"acc":acc,"rst":rst})
                break
        if name or fields:
            regs.append({"name": name or f"@0x{off:x}", "offset": off, "access":"", "reset":None,
                         "width":"", "fields": fields, "src":"detail"})
    return regs

# ---------- parse doc ----------
def parse_doc(path):
    lines = open(path).read().split("\n")
    regs = []
    cur = None
    i = 0
    while i < len(lines):
        ln = lines[i]
        mh = re.match(r"^#{3,5}\s+(?:\d+(?:\.\d+)*\s+)?`?([A-Za-z0-9_ ]+?)`?(?:\s*[—–\-][^（]*)?（?\s*[Oo]ffset\s*`?(0[xX][0-9a-fA-F]+)`?", ln)
        if mh:
            cur = {"name": mh.group(1), "offset": int(mh.group(2),16), "fields": [], "access":"", "reset":None}
            regs.append(cur); i += 1; continue
        if ln.strip().startswith("|"):
            tbl=[]
            while i < len(lines) and lines[i].strip().startswith("|"):
                tbl.append(lines[i]); i += 1
            hdr = [c.strip().lower() for c in tbl[0].strip().strip("|").split("|")]
            rows = [r for r in tbl[1:] if not re.match(r"^\|[\s:|-]+\|$", r.strip())]
            joined = "|".join(hdr)
            if hdr and hdr[0] in ("bits", "位域", "field"):
                if cur is None:
                    cur = {"name": f"<anon@{i}>", "offset": None, "fields": [], "access":"","reset":None}
                    regs.append(cur)
                bi = 0
                ni = next((k for k,h in enumerate(hdr) if h in ("name","名称","字段名")), 1)
                ai = next((k for k,h in enumerate(hdr) if h in ("r/w","access","访问","访问类型")), None)
                ri = next((k for k,h in enumerate(hdr) if "default" in h or "复位" in h or "reset" in h), None)
                di = len(hdr)-1  # description = last column
                for row in rows:
                    cells=[c.strip() for c in row.strip().strip("|").split("|")]
                    if len(cells)<=max(bi,ni): continue
                    rst = norm_rst(cells[ri]) if ri is not None and ri<len(cells) else None
                    if rst is None and di < len(cells):
                        md = re.search(r"Reset(?:\s*Value)?:\s*([^,;。<]+)", cells[di])
                        if md: rst = norm_rst(md.group(1))
                    fname = cells[ni].strip("`")
                    if ni == 1 and (hdr[ni] if ni < len(hdr) else "") in ("r/w","access"):
                        # no name column: name embedded as leading `NAME` in description
                        fname = ""
                    if not fname and di < len(cells):
                        mf = re.match(r"^`?([A-Za-z0-9_]+(?:\[[^\]]*\])?)`?\s*[—–\-]", cells[di])
                        if mf: fname = mf.group(1)
                    cur["fields"].append({
                        "bits": norm_bits(cells[bi]),
                        "name": fname,
                        "acc": norm_acc(cells[ai]) if ai is not None and ai<len(cells) else "",
                        "rst": rst,
                    })
            elif ("offset" in joined or "偏移" in joined) and ("name" in joined or "寄存器" in joined):
                # memory map table in doc
                ni = next((k for k,h in enumerate(hdr) if h in ("name","寄存器","寄存器名")), 0)
                oi = next((k for k,h in enumerate(hdr) if "offset" in h or "偏移" in h), 1)
                ai = next((k for k,h in enumerate(hdr) if h in ("access","访问","访问类型")), None)
                ri = next((k for k,h in enumerate(hdr) if "reset" in h or "复位" in h), None)
                for row in rows:
                    cells=[c.strip() for c in row.strip().strip("|").split("|")]
                    if len(cells)<=max(ni,oi): continue
                    mo = re.search(r"0[xX][0-9a-fA-F]+", cells[oi])
                    nm = cells[ni].strip("`")
                    if nm and mo:
                        regs.append({"name": nm, "offset": int(mo.group(0),16),
                                     "access": norm_acc(cells[ai]) if ai is not None and ai<len(cells) else "",
                                     "reset": norm_rst(cells[ri]) if ri is not None and ri<len(cells) else None,
                                     "fields": [], "src":"docmap"})
            continue
        i += 1
    return regs

# ---------- compare ----------
report = {}
for mod,(a,b) in CH.items():
    ug_regs = parse_ug(a,b)
    doc_regs = parse_doc(f"{ROOT}/doc_summary/module_analysis/{mod}_analysis.md")
    issues = []
    # index doc detail regs (with fields) by name and offset
    doc_by_name = {}
    doc_by_off = {}
    for r in doc_regs:
        k = norm_name(r["name"])
        # prefer the entry that carries a field table when name/offset collide
        if k not in doc_by_name or (not doc_by_name[k]["fields"] and r["fields"]):
            doc_by_name[k] = r
        if r["offset"] is not None:
            if r["offset"] not in doc_by_off or (not doc_by_off[r["offset"]]["fields"] and r["fields"]):
                doc_by_off[r["offset"]] = r
    for ur in ug_regs:
        dr = doc_by_name.get(norm_name(ur["name"])) or doc_by_off.get(ur["offset"])
        if dr is None:
            issues.append(f"MISSING reg {ur['name']} @0x{ur['offset']:x} ({ur['src']})")
            continue
        if dr["offset"] is not None and dr["offset"] != ur["offset"]:
            issues.append(f"OFFSET {ur['name']}: ug=0x{ur['offset']:x} doc=0x{dr['offset']:x}")
        if ur["access"] and dr.get("access") and norm_acc(ur["access"]) != norm_acc(dr["access"]):
            issues.append(f"ACCESS {ur['name']}: ug={ur['access']} doc={dr['access']}")
        if ur["reset"] is not None and dr.get("reset") is not None and ur["reset"] != dr["reset"]:
            issues.append(f"RESET {ur['name']}: ug={ur['reset']:#x} doc={dr['reset']:#x}")
        # field-level
        if ur["fields"] and dr["fields"]:
            dfields = {f["bits"]: f for f in dr["fields"]}
            for uf in ur["fields"]:
                if re.match(r"^\d+:\d+$|^\d+$", uf["bits"]):
                    df = dfields.get(uf["bits"])
                    if df is None:
                        # tolerate reserved-field omission
                        if "reserved" not in uf["name"].lower() and norm_name(uf["name"]) != "n/a":
                            issues.append(f"FIELD-MISS {ur['name']}[{uf['bits']}] {uf['name']}")
                        continue
                    if not names_match(uf["name"], df["name"]):
                        issues.append(f"FIELD-NAME {ur['name']}[{uf['bits']}]: ug='{uf['name']}' doc='{df['name']}'")
                    if uf["acc"] and df["acc"] and uf["acc"] != df["acc"]:
                        issues.append(f"FIELD-ACC {ur['name']}[{uf['bits']}]: ug={uf['acc']} doc={df['acc']}")
                    if uf["rst"] is not None and df["rst"] is not None and uf["rst"] != df["rst"]:
                        issues.append(f"FIELD-RST {ur['name']}[{uf['bits']}]: ug={uf['rst']:#x} doc={df['rst']:#x}")
        elif ur["fields"] and not dr["fields"]:
            issues.append(f"NO-FIELD-TABLE {ur['name']} @0x{ur['offset']:x} (ug has {len(ur['fields'])} fields)")
    report[mod] = {"ug_regs": len(ug_regs), "doc_regs": len(doc_regs), "issues": issues}

for mod, r in report.items():
    print(f"\n===== {mod}: ug_regs={r['ug_regs']} doc_regs={r['doc_regs']} issues={len(r['issues'])}")
    for s in r["issues"][:60]:
        print("  " + s)
