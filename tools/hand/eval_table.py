# THE TABLE PICCELL MADE (TSV) AGAINST THE TRUTH (ONE ROW PER LINE, CELLS SPLIT
# BY TABS OR BY " - "; "12 Top" IS TWO CELLS). PRINTS CELL AND CHARACTER
# ERRORS. USAGE: eval_table.py RESULT.TSV TRUTH.TXT
import re
import sys


def edit(a, b):
    d = list(range(len(b) + 1))
    for i in range(1, len(a) + 1):
        p, d[0] = d[0], i
        for j in range(1, len(b) + 1):
            p, d[j] = d[j], min(d[j] + 1, d[j - 1] + 1, p + (a[i - 1] != b[j - 1]))
    return d[len(b)]


def truth_rows(path):
    rows = []
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if not line.strip():
            continue
        if "\t" in line:
            rows.append(line.split("\t"))
            continue
        parts = [p.strip() for p in re.split(r"\s*-\s*" if " - " not in line else r"\s+-\s+", line)]
        # "12 Top" IS TWO CELLS (IN A LIST LINE, OR A TOTAL WITH A SPACE)
        last = re.match(r"^(\d+)\s*(\D+)$" if len(parts) > 1 else r"^(\d+)\s+(\D+)$", parts[-1])
        if last:
            parts = parts[:-1] + [last.group(1), last.group(2).strip()]
        rows.append(parts)
    return rows


def main():
    lines = [l.rstrip("\r\n").split("\t") for l in open(sys.argv[1], encoding="utf-8")]
    keep = [k for k, l in enumerate(lines) if any(c for c in l)]
    got = [lines[k] for k in keep]
    marks = None
    if len(sys.argv) > 3:
        m = [l.rstrip("\n").split("\t") for l in open(sys.argv[3])]
        marks = [m[k] if k < len(m) else [] for k in keep]
    flagged_bad = flagged_good = unflagged_bad = 0
    want = truth_rows(sys.argv[2])
    cells = bad = chars = errs = nocase = 0
    for k, w in enumerate(want):
        g0 = got[k] if k < len(got) else []
        mk0 = marks[k] if marks and k < len(marks) else []
        g = [c for c in g0 if c != ""]
        mk = [mk0[i] if i < len(mk0) else "0" for i, c in enumerate(g0) if c != ""]
        # ROWS WITH FEWER CELLS (A TITLE, A TOTAL): COMPARE THE FILLED CELLS
        for i, wc in enumerate(w):
            gc = g[i] if i < len(g) else ""
            cells += 1
            bad += gc != wc
            nocase += gc.lower() != wc.lower()
            chars += len(wc)
            errs += edit(gc, wc)
            f = marks is not None and i < len(mk) and mk[i] == "1"
            if gc != wc:
                print("  row %d col %d: %r != %r%s" % (k, i, gc, wc, " (marked)" if f else ""))
            flagged_bad += f and gc != wc
            flagged_good += f and gc == wc
            unflagged_bad += (not f) and gc != wc
    if marks is not None:
        print("marks: %d wrong cells marked, %d wrong not marked, %d right marked" % (flagged_bad, unflagged_bad, flagged_good))
    print("cells %d wrong %d (%.1f%%, %.1f%% without case)  CER %.3f" % (cells, bad, 100.0 * bad / max(1, cells), 100.0 * nocase / max(1, cells), errs / max(1, chars)))


main()
