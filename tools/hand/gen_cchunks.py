# LINE CROPS FROM SYNTHETIC PAGES, CUT BY PICCELL'S OWN C CODE (TOOLS/CUT_LINES):
# THE READER LEARNS FROM EXACTLY WHAT PICCELL.EXE GIVES IT (SHARPENED, TURNED
# BACK, MADE BIGGER). PAGES WHERE ANOTHER NUMBER OF LINES IS FOUND THAN WAS
# WRITTEN ARE SKIPPED. GEN_CCHUNKS.PY FIRST COUNT PER CUT_LINES [HARD_SHARE]
import os, sys, random, struct, subprocess, tempfile
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import synth_page

BATCH = 16


def read_lines(path):
    b = open(path, "rb").read()
    n = struct.unpack("<i", b[:4])[0]
    p, out = 4, []
    for _ in range(n):
        w = struct.unpack("<i", b[p:p + 4])[0]
        p += 4
        out.append(np.frombuffer(b[p:p + 32 * w], np.uint8).reshape(32, w).copy())
        p += 32 * w
    return out


def make(seed, count, out, cutter, hard_share):
    r = random.Random(seed)
    imgs, texts = [], []
    pages = ok = 0
    tmp = tempfile.mkdtemp(prefix="cc_")
    while len(imgs) < count:
        batch = []
        for k in range(BATCH):
            try:
                g, kind, lines = synth_page.render_page(r, hard=r.random() < hard_share)
            except Exception:
                continue
            path = os.path.join(tmp, "p%d.pgm" % k)
            Image.fromarray(g).save(path)
            batch.append((path, lines))
        pages += len(batch)
        subprocess.run([cutter] + [p for p, _ in batch], stderr=subprocess.DEVNULL)
        for path, lines in batch:
            try:
                crops = read_lines(path + ".lines")
            except Exception:
                continue
            if len(crops) != len(lines):
                continue
            ok += 1
            for a, t in zip(crops, lines):
                t = " ".join(t.replace("\t", " ").split())
                if a.shape[1] <= 1400 and t:
                    imgs.append(a)
                    texts.append(t)
    widths = np.array([i.shape[1] for i in imgs], np.int32)
    data = np.concatenate([i.reshape(-1) for i in imgs])
    np.savez_compressed(out, widths=widths, data=data, texts=np.array(texts, dtype=object))
    print("wrote", out, "pages", pages, "used", ok, flush=True)


if __name__ == "__main__":
    first, n, per, cutter = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    hard = float(sys.argv[5]) if len(sys.argv) > 5 else 0.4
    for i in range(first, first + n):
        make(9000 + i, per, os.path.join(HERE, "data", "cchunk_%04d.npz" % i), cutter, hard)
