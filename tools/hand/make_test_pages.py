# TEST PAGES WITH THEIR TRUTH, FOR TOOLS/HAND/EVAL_TABLE.PY: OUT/PAGE_N.PGM
# AND OUT/PAGE_N.TXT. USAGE: make_test_pages.py OUT COUNT [SEED] [KIND]
import os, random, sys
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth_page

out, count = sys.argv[1], int(sys.argv[2])
seed = int(sys.argv[3]) if len(sys.argv) > 3 else 777
kind = sys.argv[4] if len(sys.argv) > 4 else None
os.makedirs(out, exist_ok=True)
r = random.Random(seed)
for n in range(count):
    g, k, lines = synth_page.render_page(r, kind)
    Image.fromarray(g).save(os.path.join(out, "page_%d.pgm" % n))
    open(os.path.join(out, "page_%d.txt" % n), "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print(n, k, len(lines))
