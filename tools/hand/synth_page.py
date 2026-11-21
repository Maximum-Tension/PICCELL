# SYNTHETIC PHOTOS OF HANDWRITTEN PAGES: LISTS LIKE THE WORKER'S, TABLES WITH
# AND WITHOUT BORDERS. EVERY PAGE COMES WITH ITS LINES OF TEXT, SO THE WHOLE
# CHAIN (FINDING LINES, READING THEM, RULES) CAN BE MEASURED, AND THE LINE
# CROPS OF THE PAGE FINDER CAN BE USED FOR TRAINING.
import random, io, os, sys, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import synth


def list_lines(r, n):
    codes = [synth.rand_code(r) for _ in range(r.randint(2, 6))]
    colours = [synth.case(r, r.choice(synth.WORDS)) for _ in range(r.randint(2, 4))]
    unit = synth.case(r, r.choice(["top", "adet", "kg", "mt", "top"]))
    sep = r.choice([" - ", " - ", "-", " - ", " "])
    lines = []
    total = 0
    # A TITLE ON TOP SOMETIMES ("200'lük", A DATE, A NAME)
    if r.random() < 0.5:
        lines.append(r.choice([
            "%d'l%sk" % (r.choice([100, 150, 200, 250, 300]), r.choice(["ü", "u"])),
            "%02d.%02d.%04d" % (r.randint(1, 28), r.randint(1, 12), r.randint(2024, 2027)),
            synth.case(r, r.choice(synth.WORDS)),
        ]))
    for _ in range(n):
        q = r.randint(1, 30)
        total += q
        parts = [r.choice(codes), r.choice(colours), r.choice(colours)]
        if r.random() < 0.2:
            parts = parts[:2]
        lines.append(sep.join(parts + ["%d %s" % (q, unit)]))
    if r.random() < 0.6:
        lines.append("%d %s" % (total, unit))
    return lines


def table_lines(r, n):
    cols = r.randint(2, 5)
    kinds = [r.choice(["code", "word", "num", "price", "date"]) for _ in range(cols)]
    rows = []
    for _ in range(n):
        cells = []
        for k in kinds:
            if k == "code":
                cells.append(synth.rand_code(r))
            elif k == "word":
                cells.append(" ".join(synth.rand_word(r) for _ in range(r.randint(1, 2))))
            elif k == "num":
                cells.append(str(r.randint(0, 999)))
            elif k == "price":
                cells.append("%d,%02d" % (r.randint(0, 9999), r.randint(0, 99)))
            else:
                cells.append("%02d.%02d.%04d" % (r.randint(1, 28), r.randint(1, 12), r.randint(2020, 2027)))
        rows.append(cells)
    return rows


def render_page(r, kind=None, hard=False):
    hand, printed = synth.load_fonts()
    font_path, missing = r.choice(hand) if r.random() < 0.85 else r.choice(printed)
    size = r.randint(22, 40)
    font = ImageFont.truetype(font_path, size)
    kind = kind or r.choice(["list", "list", "table", "grid"])
    n = r.randint(6, 18)
    W = r.randint(900, 1500)
    spacing = int(size * r.uniform(1.5, 2.3))
    H = int(spacing * (n + 3) + size * 2)
    ink = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(ink)
    texts = []
    y = size * 2
    left = r.randint(20, 80)
    track = r.choice([0, 0, r.uniform(0.05, 0.3), r.uniform(0.2, 0.8)]) * size
    drawn = r.random() < 0.75
    xb = font.getbbox("x")
    pen = synth.stroke_width(font, size)
    if kind == "list":
        lines = [" ".join("".join(c for c in l if c not in missing).split()) for l in list_lines(r, n)]
        for k, l in enumerate(lines):
            x = left + r.randint(-5, 5)
            if k == len(lines) - 1 and l.count(" ") <= 1 and r.random() < 0.7:
                x = left + int(W * r.uniform(0.3, 0.5))      # THE TOTAL STANDS FURTHER RIGHT
                d.line((x - 10, y - spacing * 0.35, x + size * 4, y - spacing * 0.3), fill=255, width=max(2, size // 12))
            tilt = r.uniform(-0.03, 0.03)
            cx = x
            for ch in l:
                yy = y + (cx - x) * tilt + r.uniform(-1.5, 1.5)
                if ch == "-" and drawn:
                    # A HAND'S DASH: A LINE, OFTEN LONGER THAN THE FONT'S
                    length = r.uniform(0.3, 1.4) * size
                    cx += r.uniform(0.0, 0.25) * size
                    mid = yy + (xb[1] + xb[3]) / 2.0 + r.uniform(-0.1, 0.1) * size
                    # AS THICK AS THE LETTERS OR MORE: A THINNER ONE VANISHES WHEN THE PAGE IS SMALL
                    d.line((cx, mid, cx + length, mid + r.uniform(-0.1, 0.1) * length), fill=255, width=max(2, int(round(pen * r.uniform(1.0, 1.5)))))
                    cx += length + r.uniform(0.0, 0.25) * size
                    continue
                d.text((cx, yy), ch, fill=255, font=font)
                cx += font.getlength(ch) + (track * r.uniform(0.5, 1.5) if ch != " " else track * 0.5)
            texts.append(l)
            y += spacing + r.randint(-3, 3)
        cells = None
    else:
        rows = table_lines(r, n)
        rows = [[" ".join("".join(c for c in cell if c not in missing).split()) or "0" for cell in row] for row in rows]
        widths = [max(font.getlength(row[c]) for row in rows) + size * r.uniform(1.0, 2.5) for c in range(len(rows[0]))]
        tot = sum(widths)
        if left + tot > W - 20:
            W = int(left + tot + 40)
            ink = ink.resize((W, H)) if False else Image.new("L", (W, H), 0)
            d = ImageDraw.Draw(ink)
        for row in rows:
            x = left
            for c, cell in enumerate(row):
                d.text((x + r.uniform(-3, 3), y + r.uniform(-2, 2)), cell, fill=255, font=font)
                x += widths[c]
            texts.append("\t".join(row))
            y += spacing
        if kind == "grid":
            xs = [left - size * 0.4]
            for wv in widths:
                xs.append(xs[-1] + wv)
            # THE RULES GO BETWEEN THE ROWS, NOT THROUGH THE LETTERS
            bb = font.getbbox("Hg")
            y0 = size * 2 + (bb[1] + bb[3]) / 2.0 - spacing * 0.5
            for k in range(len(rows) + 1):
                yy = y0 + k * spacing
                d.line((xs[0], yy, xs[-1], yy), fill=255, width=2)
            for xx in xs:
                d.line((xx, y0, xx, y0 + len(rows) * spacing), fill=255, width=2)
        cells = rows
    a = np.asarray(ink).astype(np.float32) / 255.0
    # PAPER, LIGHT, PERSPECTIVE-ISH TURN, BLUR, NOISE, JPEG
    paper = r.uniform(0.75, 1.0)
    inkc = r.uniform(0.0, 0.35)
    gx = np.linspace(0, 1, W)[None, :] * r.uniform(-0.25, 0.25)
    gy = np.linspace(0, 1, H)[:, None] * r.uniform(-0.25, 0.25)
    bg = np.clip(paper + gx + gy, 0.35, 1.0)
    img = bg * (1 - a) + inkc * a
    im = Image.fromarray(np.clip(img * 255, 0, 255).astype(np.uint8))
    # HARD PAGES: SMALLER, SOFTER AND MORE TURNED, LIKE A QUICK PHONE PHOTO
    turn = 5.0 if hard else 2.0
    im = im.rotate(r.uniform(-turn, turn), Image.BILINEAR, expand=True, fillcolor=int(paper * 230))
    scale = r.uniform(0.3, 0.8) if hard else r.uniform(0.45, 1.0)
    im = im.resize((int(im.width * scale), int(im.height * scale)), Image.BILINEAR)
    if hard and r.random() < 0.85:
        im = im.filter(ImageFilter.GaussianBlur(r.uniform(0.4, 1.4) * max(0.6, scale * 1.6)))
    elif r.random() < 0.5:
        im = im.filter(ImageFilter.GaussianBlur(r.uniform(0.3, 1.0)))
    arr = np.asarray(im).astype(np.float32) + np.random.default_rng(r.randint(0, 1 << 30)).normal(0, r.uniform(0, 8), (im.height, im.width))
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))
    buf = io.BytesIO()
    im.save(buf, "JPEG", quality=r.randint(40, 92))
    im = Image.open(io.BytesIO(buf.getvalue())).convert("L")
    return np.asarray(im), kind, texts


if __name__ == "__main__":
    r = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
    for k in range(int(sys.argv[2]) if len(sys.argv) > 2 else 4):
        g, kind, texts = render_page(r)
        Image.fromarray(g).save("/tmp/page_%d.png" % k)
        print(kind, len(texts), texts[:3])
