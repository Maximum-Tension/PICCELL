# TEST PAGES OF TABLES DRAWN BY HAND: WOBBLY, SLANTED, PALE OR DARK LINES
# THAT OVERSHOOT THE CORNERS, ONE OR TWO TABLES ON A PAGE, SOMETIMES A LINE
# OF TEXT UNDER THEM. CELLS: TIMES, SMALL NUMBERS, CODES, WORDS. THE TRUTH
# HAS A ROW PER LINE AND AN EMPTY LINE BETWEEN THE TABLES.
# USAGE: gen_drawn.py OUT COUNT [SEED]
import io, os, random, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import synth

HEADERS = ["SAAT", "VARDIYA", "MASA", "PART", "ADET", "KOD", "AD", "DURUM",
           "NOT", "GUN", "SIRA", "BOLUM", "Saat", "Masa", "Adet", "Kod",
           "Durum", "Not", "Hat", "Oda", "Kat", "Plan"]


def cell(r, kind):
    if kind == "time":
        return "%02d:%02d" % (r.randint(0, 23), r.choice([0, 0, 15, 30, 30, 45, r.randint(0, 59)]))
    if kind == "small":
        return str(r.randint(1, 9 if r.random() < 0.6 else 99))
    if kind == "code":
        a = r.choice("ABCDEFGHKLMNPRSTUVYZ")
        if r.random() < 0.5:
            return a + str(r.randint(1, 9))
        return a + r.choice("ABCDEFGHKLMNPRSTUVYZ")
    w = synth.rand_word(r)
    if r.random() < 0.4:
        w += " " + synth.rand_word(r)
    return w


def wobbly(d, a, b, width, fill, r):
    # A LINE DRAWN BY HAND: SMALL BENDS EVERY 30-60 PIXELS
    (x0, y0), (x1, y1) = a, b
    n = max(2, int(max(abs(x1 - x0), abs(y1 - y0)) / r.uniform(30, 60)))
    pts = []
    for k in range(n + 1):
        t = k / n
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        if 0 < k < n:
            if abs(x1 - x0) > abs(y1 - y0):
                y += r.uniform(-2.5, 2.5)
            else:
                x += r.uniform(-2.5, 2.5)
        pts.append((x, y))
    d.line(pts, fill=fill, width=width, joint="curve")


def draw_table(d, r, font, size, pen, x0, y0, missing):
    cols = r.randint(2, 4)
    kinds = [r.choice(["time", "small", "code", "word"]) for _ in range(cols)]
    rows = r.randint(2, 5)
    head = [r.choice(HEADERS) for _ in range(cols)]
    body = [[cell(r, k) for k in kinds] for _ in range(rows)]
    table = [head] + body
    table = [[" ".join("".join(c for c in x if c not in missing).split()) or "0" for x in row] for row in table]
    widths = [max(font.getlength(row[c]) for row in table) + size * r.uniform(1.2, 3.0) for c in range(cols)]
    heights = [size * r.uniform(1.8, 3.0) for _ in table]
    slope = r.uniform(-0.02, 0.02)
    width = max(2, int(round(pen * r.uniform(0.8, 1.6))))
    fill = r.choice([255, 255, 220, 170])
    xs = [x0]
    for w in widths:
        xs.append(xs[-1] + w)
    ys = [y0]
    for h in heights:
        ys.append(ys[-1] + h)
    over = lambda: r.uniform(0, size * 0.5)
    for y in ys:
        a = (xs[0] - over(), y + slope * (xs[0] - x0))
        b = (xs[-1] + over(), y + slope * (xs[-1] - x0) + r.uniform(-3, 3))
        wobbly(d, a, b, width, fill, r)
    for x in xs:
        a = (x + r.uniform(-3, 3), ys[0] - over() + slope * (x - x0))
        b = (x + r.uniform(-6, 6), ys[-1] + over() + slope * (x - x0))
        wobbly(d, a, b, width, fill, r)
    bb = font.getbbox("Hg")
    for i, row in enumerate(table):
        for c, text in enumerate(row):
            tw = font.getlength(text)
            room = widths[c] - tw
            x = xs[c] + r.uniform(0.15, 0.85) * room
            # IN THE CELL, SOMETIMES CLOSE TO THE LINE UNDER IT
            free = heights[i] - (bb[3] - bb[1])
            y = ys[i] + r.uniform(0.2, 0.9) * free - bb[1] + slope * (x - x0)
            d.text((x, y), text, fill=255, font=font)
    return table, ys[-1]


def render(r):
    hand, printed = synth.load_fonts()
    font_path, missing = r.choice(hand)
    size = r.randint(24, 40)
    font = ImageFont.truetype(font_path, size)
    pen = synth.stroke_width(font, size)
    W, H = 1400, 2000
    ink = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(ink)
    truth = []
    y = r.randint(60, 160)
    for t in range(r.choice([1, 1, 2])):
        table, bottom = draw_table(d, r, font, size, pen, r.randint(60, 300), y, missing)
        if truth:
            truth.append("")
        truth += ["\t".join(row) for row in table]
        y = bottom + size * r.uniform(3, 6)
    if r.random() < 0.3:
        line = " ".join(synth.rand_word(r) for _ in range(r.randint(2, 5)))
        line = " ".join("".join(c for c in line if c not in missing).split())
        if line:
            d.text((r.randint(60, 300), y), line, fill=255, font=font)
            truth += ["", line]
            y += size * 2
    H = int(min(H, y + size * 2))
    ink = ink.crop((0, 0, W, H))
    a = np.asarray(ink).astype(np.float32) / 255.0
    paper = r.uniform(0.75, 1.0)
    inkc = r.uniform(0.0, 0.3)
    gx = np.linspace(0, 1, W)[None, :] * r.uniform(-0.2, 0.2)
    gy = np.linspace(0, 1, H)[:, None] * r.uniform(-0.2, 0.2)
    bg = np.clip(paper + gx + gy, 0.35, 1.0)
    img = bg * (1 - a) + inkc * a
    im = Image.fromarray(np.clip(img * 255, 0, 255).astype(np.uint8))
    im = im.rotate(r.uniform(-2.0, 2.0), Image.BILINEAR, expand=True, fillcolor=int(paper * 230))
    scale = r.uniform(0.5, 1.0)
    im = im.resize((int(im.width * scale), int(im.height * scale)), Image.BILINEAR)
    if r.random() < 0.5:
        im = im.filter(ImageFilter.GaussianBlur(r.uniform(0.3, 0.9)))
    arr = np.asarray(im).astype(np.float32)
    arr += np.random.default_rng(r.randint(0, 1 << 30)).normal(0, r.uniform(0, 6), arr.shape)
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))
    buf = io.BytesIO()
    im.save(buf, "JPEG", quality=r.randint(50, 92))
    im = Image.open(io.BytesIO(buf.getvalue())).convert("L")
    return np.asarray(im), truth


if __name__ == "__main__":
    out, count = sys.argv[1], int(sys.argv[2])
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 4242
    os.makedirs(out, exist_ok=True)
    r = random.Random(seed)
    for n in range(count):
        g, truth = render(r)
        Image.fromarray(g).save(os.path.join(out, "page_%d.pgm" % n))
        open(os.path.join(out, "page_%d.txt" % n), "w", encoding="utf-8").write("\n".join(truth) + "\n")
        print(n, len(truth))
