# LINE CROPS FROM SYNTHETIC PAGES, CUT BY THE PAGE FINDER ITSELF: THE READER
# LEARNS FROM EXACTLY WHAT IT WILL SEE. PAGES WHERE THE FINDER GETS ANOTHER
# NUMBER OF LINES THAN WERE WRITTEN ARE SKIPPED.
import os, sys, random
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import synth_page, page

HARD = os.environ.get("HARD", "") == "1"


def make(seed, count, out):
    r = random.Random(seed)
    imgs, texts = [], []
    pages = ok = 0
    while len(imgs) < count:
        pages += 1
        try:
            g, kind, lines = synth_page.render_page(r, hard=HARD)
            crops = page.line_crops_strips(g)
        except Exception as e:
            continue
        if len(crops) != len(lines):
            continue
        ok += 1
        for (box, a), t in zip(crops, lines):
            t = " ".join(t.replace("\t", " ").split())
            if a.shape[1] <= 1400 and t:
                imgs.append(a); texts.append(t)
    widths = np.array([i.shape[1] for i in imgs], np.int32)
    data = np.concatenate([i.reshape(-1) for i in imgs])
    np.savez_compressed(out, widths=widths, data=data, texts=np.array(texts, dtype=object))
    print("wrote", out, "pages", pages, "used", ok, flush=True)

if __name__ == "__main__":
    first, n, per = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
    outdir = sys.argv[4] if len(sys.argv) > 4 else os.path.join(HERE, "data")
    os.makedirs(outdir, exist_ok=True)
    for i in range(first, first + n):
        make(5000 + i, per, os.path.join(outdir, "pchunk_%04d.npz" % i))
