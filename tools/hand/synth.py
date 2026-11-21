# SYNTHETIC TEXT LINES FOR TRAINING: HANDWRITING FONTS (AND SOME PRINTED ONES)
# DRAWN WITH RANDOM SLANT, WAVES, THICKNESS, BLUR, NOISE, PAPER AND JPEG.
# EVERY SAMPLE IS A GRAYSCALE LINE, HEIGHT 32, TEXT DARK ON LIGHT.
import io, os, json, random, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
HEIGHT = 32

CHARSET = (
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "ÇĞİÖŞÜçğıöşü"
    " .,-/:()%+'\"&#*x=;!?"
)
CHARSET = "".join(dict.fromkeys(CHARSET))  # NO DOUBLES ("x" IS ALREADY IN)
TR = set("ÇĞİÖŞÜçğıöşü")

WORDS = """
cream gray grey vizon black white red blue green yellow brown beige navy pink
purple orange silver gold ecru bej krem gri siyah beyaz kırmızı mavi yeşil sarı
kahve kahverengi lacivert pembe mor turuncu gümüş altın ekru antrasit bordo
haki füme vizon taba hardal mint lila somon fuşya petrol indigo natural
top adet kg metre mt paket koli kutu rulo çift takım tane litre gram
toplam ara toplam tarih fiyat tutar miktar birim kod ürün renk açıklama
sipariş fatura irsaliye müşteri tedarikçi depo stok giriş çıkış iade not
pazartesi salı çarşamba perşembe cuma cumartesi pazar ocak şubat mart nisan
mayıs haziran temmuz ağustos eylül ekim kasım aralık
kumaş iplik pamuk polyester viskon keten yün ipek elastan likra penye süprem
ribana interlok kaşkorse örme dokuma boya baskı yıkama apre ham hazır
total sum date price amount quantity unit code product color description
order invoice customer supplier stock in out return note page sheet
monday tuesday wednesday thursday friday saturday sunday
cotton wool silk linen roll box pack pair set piece meter
ali veli ayşe fatma mehmet ahmet mustafa zeynep elif emre can deniz teoman
istanbul ankara izmir bursa adana konya antalya kayseri denizli gaziantep
ve ile için bir iki üç dört beş altı yedi sekiz dokuz on yüz bin
the and for with from to of in on at by new old big small
""".split()

PRINTED = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/truetype/google-fonts/Poppins-Regular.ttf",
]


def coverage(path):
    from fontTools.ttLib import TTFont
    cmap = TTFont(path, fontNumber=0).getBestCmap() or {}
    return set(c for c in CHARSET if ord(c) not in cmap and c != " ")


_FONTS = None


def load_fonts():
    global _FONTS
    if _FONTS is not None:
        return _FONTS
    cache = os.path.join(HERE, "meta", "coverage2.json")
    cov = json.load(open(cache)) if os.path.exists(cache) else {}
    hand, printed = [], []
    for d, dst in (("fonts", hand), ("fonts_print", printed)):
        for f in sorted(os.listdir(os.path.join(HERE, d))):
            path = os.path.join(HERE, d, f)
            if path not in cov:
                cov[path] = "".join(sorted(coverage(path)))
            dst.append((path, set(cov[path])))
    for path in PRINTED:
        if os.path.exists(path):
            if path not in cov:
                cov[path] = "".join(sorted(coverage(path)))
            printed.append((path, set(cov[path])))
    json.dump(cov, open(cache, "w"), indent=1)
    _FONTS = (hand, printed)
    return _FONTS


# ---------------------------------------------------------------- TEXT
def rand_code(r):
    k = r.random()
    if k < 0.45:
        return str(r.randint(0, 99999)).zfill(r.choice([4, 4, 5, 5, 3]))
    if k < 0.65:
        return "".join(r.choice("ABCDEFGHJKLMNPRSTUVYZ") for _ in range(r.randint(1, 4))) + \
            r.choice(["", " ", "-"]) + str(r.randint(1, 9999))
    if k < 0.8:
        return "%d.%d" % (r.randint(1, 999), r.randint(0, 99))
    return str(r.randint(1, 999))


def case(r, w):
    k = r.random()
    if k < 0.5:
        return w
    if k < 0.75:
        return w[:1].upper() + w[1:]
    if k < 0.9:
        return w.upper().replace("İ", "İ")
    return "".join(c.upper() if r.random() < 0.3 else c for c in w)


def rand_word(r):
    return case(r, r.choice(WORDS))


def rand_text(r):
    k = r.random()
    sep = r.choice([" - ", " - ", "-", " -", "- ", "  -  ", " — ", " "])
    if k < 0.30:  # THE WORKER'S LIST: CODE - COLOUR - COLOUR - N TOP
        parts = [rand_code(r), rand_word(r), rand_word(r)]
        if r.random() < 0.3:
            parts = parts[:2]
        n = "%d %s" % (r.randint(1, 250), case(r, r.choice(["top", "adet", "kg", "mt", "top", "top"])))
        return sep.join(parts + [n])
    if k < 0.50:  # WORDS
        return " ".join(rand_word(r) for _ in range(r.randint(1, 4)))
    if k < 0.65:  # NUMBERS, PRICES, DATES
        f = r.choice([
            lambda: str(r.randint(0, 999999)),
            lambda: "%d,%02d" % (r.randint(0, 99999), r.randint(0, 99)),
            lambda: "%d.%03d,%02d" % (r.randint(1, 999), r.randint(0, 999), r.randint(0, 99)),
            lambda: "%02d.%02d.%04d" % (r.randint(1, 31), r.randint(1, 12), r.randint(2000, 2030)),
            lambda: "%02d/%02d/%02d" % (r.randint(1, 31), r.randint(1, 12), r.randint(0, 99)),
            lambda: "%d%%" % r.randint(0, 100),
            lambda: "%d x %d" % (r.randint(1, 99), r.randint(1, 99)),
            lambda: "(%d)" % r.randint(1, 999),
        ])
        return " ".join(f() for _ in range(r.randint(1, 3)))
    if k < 0.80:  # MIXED CELLS
        return " ".join(r.choice([rand_code(r), rand_word(r), str(r.randint(0, 999))]) for _ in range(r.randint(2, 5)))
    # RANDOM CHARACTERS: NO LANGUAGE TO LEAN ON
    n = r.randint(3, 14)
    s = "".join(r.choice(CHARSET[:-1] if r.random() < 0.9 else CHARSET) for _ in range(n))
    return " ".join(s.split()) or "a"


# ---------------------------------------------------------------- DRAWING
def stroke_width(font, size):
    # THE PEN OF A FONT: HOW WIDE ITS "l" (OR "I") IS IN THE MIDDLE
    for ch in "lI1":
        b = font.getbbox(ch)
        if b[2] <= b[0]:
            continue
        im = Image.new("L", (b[2] + 4, b[3] + 4), 0)
        ImageDraw.Draw(im).text((2, 2), ch, fill=255, font=font)
        a = np.asarray(im) > 127
        row = a[(b[1] + b[3]) // 2 + 2]
        if row.any():
            return max(1, int(row.sum()))
    return max(1, size // 12)


def render(r, font_path, missing, text):
    text = "".join(c for c in text if c not in missing)
    text = " ".join(text.split())
    if not text:
        text = str(r.randint(0, 99))
    size = r.randint(28, 48)
    font = ImageFont.truetype(font_path, size)
    try:
        axes = font.get_variation_axes()
        vals = []
        for ax in axes:
            name = ax.get("name", b"")
            name = name.decode() if isinstance(name, bytes) else str(name)
            if "eight" in name or name.lower().startswith("wght"):
                vals.append(r.uniform(max(ax["minimum"], 300), min(ax["maximum"], 800)))
            else:
                vals.append(ax["default"])
        font.set_variation_by_axes(vals)
    except Exception:
        pass
    box = font.getbbox(text)
    track = r.choice([0, 0, 0, r.uniform(0, 0.35), r.uniform(0.1, 0.6), r.uniform(0.2, 0.9)]) * size
    jitter = r.uniform(0, 0.08) * size
    # HANDS DRAW A DASH AS A LINE, OFTEN A LONG ONE, NOT AS THE FONT'S GLYPH
    drawn = "-" in text and r.random() < 0.75
    w = int(box[2] - box[0] + size * 2 + track * len(text) + jitter * 2 + (size * 1.6 * text.count("-") if drawn else 0))
    h = box[3] - box[1] + size * 2
    img = Image.new("L", (max(w, 8), max(h, 8)), 0)
    d = ImageDraw.Draw(img)
    if track == 0 and jitter < 0.02 * size and not drawn:
        d.text((size - box[0], size - box[1]), text, fill=255, font=font)
    else:
        # LETTER BY LETTER: WIDER SPACING AND A LITTLE UP AND DOWN, LIKE A HAND
        x = size - box[0]
        xb = font.getbbox("x")
        pen = stroke_width(font, size)
        for ch in text:
            dy = r.uniform(-jitter, jitter)
            if ch == "-" and drawn:
                length = r.uniform(0.3, 1.4) * size
                x += r.uniform(0.0, 0.25) * size
                mid = size - box[1] + dy + (xb[1] + xb[3]) / 2.0 + r.uniform(-0.12, 0.12) * size
                tilt = r.uniform(-0.1, 0.1) * length
                d.line([(x, mid), (x + length, mid + tilt)], fill=255, width=max(1, int(round(pen * r.uniform(0.7, 1.3)))))
                x += length + r.uniform(0.0, 0.25) * size
                continue
            d.text((x, size - box[1] + dy), ch, fill=255, font=font)
            x += font.getlength(ch) + (track * r.uniform(0.6, 1.4) if ch != " " else track * 0.5)
    a = np.asarray(img).astype(np.float32) / 255.0

    # THIN FONTS GET A THICKER PEN FIRST: AT 32 PIXELS THEY WOULD VANISH
    for _ in range(3):
        ys, xs = np.nonzero(a > 0.5)
        if len(ys) == 0:
            break
        dens = len(ys) / float((ys.max() - ys.min() + 1) * (xs.max() - xs.min() + 1))
        if dens >= 0.10:
            break
        a = np.asarray(Image.fromarray((a * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(3))) / 255.0

    # THICKNESS: THINNER OR THICKER PEN
    k = r.random()
    if k < 0.25:
        a = np.asarray(Image.fromarray((a * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(3))) / 255.0
    elif k < 0.40:
        b = np.asarray(Image.fromarray((a * 255).astype(np.uint8)).filter(ImageFilter.MinFilter(3))) / 255.0
        if (b > 0.5).sum() > 0.6 * (a > 0.5).sum():
            a = b

    # SLANT AND A LITTLE ROTATION
    shear = r.uniform(-0.45, 0.35)
    hh, ww = a.shape
    im = Image.fromarray((a * 255).astype(np.uint8))
    im = im.transform((ww + int(abs(shear) * hh), hh), Image.AFFINE,
                      (1, shear, -abs(shear) * hh if shear > 0 else 0, 0, 1, 0), Image.BILINEAR)
    im = im.rotate(r.uniform(-2.5, 2.5), Image.BILINEAR, expand=True)
    a = np.asarray(im).astype(np.float32) / 255.0

    # WAVES: THE BASELINE AND THE LETTER WIDTHS WANDER
    hh, ww = a.shape
    if r.random() < 0.7:
        amp = r.uniform(0, 0.06) * size
        per = r.uniform(1.5, 6) * size
        ph = r.uniform(0, 6.3)
        ys = np.arange(hh)[:, None]
        xs = np.arange(ww)[None, :]
        dy = amp * np.sin(xs / per * 6.283 + ph)
        dx = r.uniform(0, 0.04) * size * np.sin(ys / (0.7 * size) * 6.283 + ph)
        src_y = np.clip(ys + dy, 0, hh - 1).astype(np.int32)
        src_x = np.clip(xs + dx, 0, ww - 1).astype(np.int32)
        a = a[src_y, src_x]

    # CROP TO THE INK WITH RANDOM MARGINS
    ys, xs = np.nonzero(a > 0.2)
    if len(ys) == 0:
        return None
    y0, y1, x0, x1 = ys.min(), ys.max(), xs.min(), xs.max()
    th = y1 - y0 + 1
    my = lambda: int(r.uniform(0.02, 0.25) * th)
    mx = lambda: int(r.uniform(0.0, 0.6) * th)
    y0 = max(0, y0 - my()); y1 = min(a.shape[0] - 1, y1 + my())
    x0 = max(0, x0 - mx()); x1 = min(a.shape[1] - 1, x1 + mx())
    a = a[y0:y1 + 1, x0:x1 + 1]

    # INK AND PAPER
    paper = r.uniform(0.62, 1.0)
    ink = r.uniform(0.0, min(0.45, paper - 0.25))
    hh, ww = a.shape
    bg = np.full((hh, ww), paper, np.float32)
    if r.random() < 0.6:  # UNEVEN LIGHT
        gx = np.linspace(0, 1, ww)[None, :] * r.uniform(-0.25, 0.25)
        gy = np.linspace(0, 1, hh)[:, None] * r.uniform(-0.2, 0.2)
        bg = np.clip(bg + gx + gy, 0.3, 1.0)
    if r.random() < 0.15:  # A RULED LINE OR A TABLE BORDER CLOSE BY
        y = r.choice([r.randint(0, max(0, hh // 6)), r.randint(hh - 1 - hh // 6, hh - 1)])
        bg[y:y + r.randint(1, 2), :] *= r.uniform(0.4, 0.8)
    out = bg * (1 - a) + ink * a

    # RESIZE TO HEIGHT 32 THROUGH A RANDOM LOWER RESOLUTION (PHONE PHOTOS)
    im = Image.fromarray(np.clip(out * 255, 0, 255).astype(np.uint8))
    if r.random() < 0.5:
        lo = r.randint(14, 30)
        im = im.resize((max(4, int(ww * lo / hh)), lo), Image.BILINEAR)
    im = im.resize((max(8, int(round(ww * HEIGHT / hh * r.uniform(0.85, 1.15)))), HEIGHT), Image.BILINEAR)
    if r.random() < 0.4:
        im = im.filter(ImageFilter.GaussianBlur(r.uniform(0.3, 1.0)))
    a = np.asarray(im).astype(np.float32)
    a += np.random.default_rng(r.randint(0, 1 << 30)).normal(0, r.uniform(0, 10), a.shape)
    # THE READER STRETCHES EVERY CROP TO FULL CONTRAST: SO DOES THE TRAINING
    lo, hi = np.percentile(a, 1), np.percentile(a, 99)
    if hi - lo < 40:
        return None
    if r.random() < 0.4:
        # LIKE THE PAGE READER'S CROPS: PAPER WHITE, INK DARK, SOFT EDGES
        a = np.clip(a, 0, 255).astype(np.uint8)
        bg = np.asarray(Image.fromarray(a).filter(ImageFilter.MaxFilter(r.choice([7, 9, 11, 13]))).filter(ImageFilter.GaussianBlur(4))).astype(np.float32)
        n = np.clip(a.astype(np.float32) / np.maximum(bg, 1), 0, 1)
        ink = np.clip((r.uniform(0.82, 0.92) - n) / r.uniform(0.25, 0.45), 0, 1)
        if ink.max() < 0.3:
            return None
        a = (1.0 - ink) * 255.0
    else:
        a = (a - lo) / (hi - lo) * r.uniform(170, 250) + r.uniform(0, 40)
    im = Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))
    if r.random() < 0.6:
        buf = io.BytesIO()
        im.save(buf, "JPEG", quality=r.randint(25, 90))
        im = Image.open(io.BytesIO(buf.getvalue())).convert("L")
    return np.asarray(im), text


def sample(r, hand, printed):
    for _ in range(10):
        font_path, missing = r.choice(printed) if r.random() < 0.2 else r.choice(hand)
        try:
            res = render(r, font_path, missing, rand_text(r))
        except Exception:
            res = None
        if res is not None and res[0].shape[1] <= 1024:
            return res
    return None


def make_chunk(args):
    seed, count, out = args
    r = random.Random(seed)
    hand, printed = load_fonts()
    imgs, texts = [], []
    while len(imgs) < count:
        s = sample(r, hand, printed)
        if s is not None:
            imgs.append(s[0]); texts.append(s[1])
    widths = np.array([i.shape[1] for i in imgs], np.int32)
    data = np.concatenate([i.reshape(-1) for i in imgs])
    np.savez_compressed(out, widths=widths, data=data, texts=np.array(texts, dtype=object))
    return out


if __name__ == "__main__":
    import sys, multiprocessing as mp
    first, n_chunks, per = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
    os.makedirs(os.path.join(HERE, "data"), exist_ok=True)
    jobs = [(1000 + i, per, os.path.join(HERE, "data", "chunk_%04d.npz" % i)) for i in range(first, first + n_chunks)]
    with mp.Pool(2) as p:
        for o in p.imap_unordered(make_chunk, jobs):
            print("wrote", o, flush=True)
