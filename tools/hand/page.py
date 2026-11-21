# A PHOTO OF A HANDWRITTEN PAGE -> TEXT LINES (HEIGHT 32, READY FOR THE READER)
import numpy as np
from PIL import Image, ImageFilter


def ink_map(gray):
    # PAPER = 0, INK = 1, WITH THE UNEVEN LIGHT TAKEN OUT
    g = gray.astype(np.float32)
    bg = np.asarray(Image.fromarray(gray).filter(ImageFilter.MaxFilter(15)).filter(ImageFilter.GaussianBlur(10))).astype(np.float32)
    n = np.clip(g / np.maximum(bg, 1), 0, 1)       # 1 = PAPER
    ink = np.clip((0.88 - n) / 0.35, 0, 1)          # DARKER THAN 88% OF THE PAPER
    return ink


def erase_rules(ink):
    # LONG HORIZONTAL STROKES (UNDERLINES, TABLE BORDERS) ARE NOT TEXT
    h, w = ink.shape
    b = ink > 0.5
    out = ink.copy()
    limit = max(40, w // 6)
    for y in range(h):
        row = b[y]
        x = 0
        while x < w:
            if row[x]:
                s = x
                while x < w and row[x]:
                    x += 1
                if x - s >= limit:
                    out[max(0, y - 1):y + 2, s:x] = 0
            else:
                x += 1
    return out


def find_lines(ink, min_h=8):
    prof = ink.sum(1)
    sm = np.convolve(prof, np.ones(3) / 3, mode="same")
    thr = max(1.0, 0.08 * sm.max())
    on = sm > thr
    bands = []
    y = 0
    h = len(on)
    while y < h:
        if on[y]:
            s = y
            while y < h and on[y]:
                y += 1
            if y - s >= min_h:
                bands.append([s, y])
        else:
            y += 1
    # A BAND TWICE AS TALL AS USUAL IS TWO LINES: CUT AT ITS WEAKEST ROW
    if bands:
        med = np.median([b[1] - b[0] for b in bands])
        out = []
        for s, e in bands:
            if e - s > 1.7 * med:
                n = int(round((e - s) / med))
                cuts = [s]
                for k in range(1, n):
                    c = s + int((e - s) * k / n)
                    lo, hi = max(s + 2, c - int(med / 3)), min(e - 2, c + int(med / 3))
                    cuts.append(lo + int(np.argmin(sm[lo:hi])) if hi > lo else c)
                cuts.append(e)
                out += [[cuts[i], cuts[i + 1]] for i in range(len(cuts) - 1)]
            else:
                out.append([s, e])
        bands = out
    return bands


def line_crops(gray, pad_frac=0.25):
    ink = erase_rules(ink_map(gray))
    bands = find_lines(ink)
    crops = []
    for s, e in bands:
        hgt = e - s
        y0 = max(0, s - int(pad_frac * hgt))
        y1 = min(gray.shape[0], e + int(pad_frac * hgt))
        band = ink[y0:y1]
        cols = np.nonzero(band.sum(0) > 0.5)[0]
        if len(cols) == 0:
            continue
        x0 = max(0, cols.min() - 4)
        x1 = min(gray.shape[1], cols.max() + 5)
        piece = 1.0 - band[:, x0:x1]                  # PAPER = 1
        a = piece * 255.0
        lo, hi = np.percentile(a, 1), np.percentile(a, 99)
        a = np.clip((a - lo) / max(1.0, hi - lo) * 255.0, 0, 255).astype(np.uint8)
        im = Image.fromarray(a)
        im = im.resize((max(8, int(round(im.width * 32.0 / im.height))), 32), Image.BILINEAR)
        crops.append(((x0, y0, x1, y1), np.asarray(im)))
    return crops


if __name__ == "__main__":
    import sys
    g = np.asarray(Image.open(sys.argv[1]).convert("L"))
    cs = line_crops(g)
    W = max(c[1].shape[1] for c in cs)
    canvas = np.full((len(cs) * 36, W), 255, np.uint8)
    for k, (_, a) in enumerate(cs):
        canvas[k * 36:k * 36 + 32, :a.shape[1]] = a
    Image.fromarray(canvas).save(sys.argv[2])
    print(len(cs), "lines")


# ---------------------------------------------------------------- LINES FROM PIECES OF INK
def line_crops_cc(gray):
    from scipy import ndimage
    ink = erase_rules(ink_map(gray))
    h, w = ink.shape
    b = ink > 0.35
    lab, n = ndimage.label(b, structure=np.ones((3, 3)))
    objs = ndimage.find_objects(lab)
    comps = []
    for k, sl in enumerate(objs):
        if sl is None:
            continue
        y0, y1, x0, x1 = sl[0].start, sl[0].stop, sl[1].start, sl[1].stop
        area = int((lab[sl] == k + 1).sum())
        if area < 6:
            continue
        # THE DARK EDGE OF THE PHOTO: TALL AND TOUCHING THE BORDER
        if (x0 == 0 or x1 == w or y0 == 0 or y1 == h) and (y1 - y0 > h / 4 or x1 - x0 > w / 3):
            continue
        comps.append(dict(id=k + 1, x0=x0, x1=x1, y0=y0, y1=y1, cx=(x0 + x1) / 2, cy=(y0 + y1) / 2, h=y1 - y0, area=area))
    hs = sorted(c["h"] for c in comps if c["area"] > 20)
    H = hs[len(hs) // 2] if hs else 10
    big = [c for c in comps if c["h"] >= 0.45 * H]
    small = [c for c in comps if c["h"] < 0.45 * H]
    # CHAINS: EVERY BIG PIECE GETS AT MOST ONE NEIGHBOUR ON EACH SIDE. THE BEST
    # LINKS (SAME HEIGHT, SMALL GAP) ARE TAKEN FIRST. FAR APART PIECES MAY DRIFT
    # A LITTLE MORE UP OR DOWN: HANDWRITTEN LINES WANDER.
    links = []
    for i, c in enumerate(big):
        for j, d in enumerate(big):
            if j == i or d["cx"] <= c["cx"] or d["x0"] <= c["x0"]:
                continue
            gap = d["x0"] - c["x1"]
            if gap < -0.5 * H:
                continue
            dy = abs(d["cy"] - c["cy"])
            if dy > 0.5 * H + 0.1 * max(gap, 0):
                continue
            links.append((dy * 2.0 + 0.05 * max(gap, 0), i, j))
    links.sort()
    right = [-1] * len(big)
    left = [-1] * len(big)
    parent = list(range(len(big)))

    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for cost, i, j in links:
        if right[i] >= 0 or left[j] >= 0 or root(i) == root(j):
            continue
        right[i] = j
        left[j] = i
        parent[root(i)] = root(j)
    groups = {}
    for i, c in enumerate(big):
        groups.setdefault(root(i), []).append(c)
    lines = [g for g in groups.values()]
    # SHORT GROUPS (A LONE "T" BAR, THE TOP OF A "P") JOIN A REAL LINE LATER
    real = [g for g in lines if max(d["x1"] for d in g) - min(d["x0"] for d in g) >= 3 * H]
    loose = [d for g in lines if g not in real for d in g]
    lines = real
    # A LINE: CENTRES FITTED BY A STRAIGHT LINE
    fits = []
    for g in lines:
        xs = np.array([c["cx"] for c in g]); ys = np.array([c["cy"] for c in g])
        if len(g) >= 2 and xs.max() - xs.min() > 2 * H:
            a, b0 = np.polyfit(xs, ys, 1)
        else:
            a, b0 = 0.0, float(ys.mean())
        fits.append((a, b0))
    small = small + loose
    # SMALL PIECES (DASHES, DOTS, COMMAS) GO TO THE LINE THEY SIT IN
    for c in small:
        best, bd = -1, 1e9
        for k, (g, (a, b0)) in enumerate(zip(lines, fits)):
            gx0 = min(d["x0"] for d in g) - 3 * H; gx1 = max(d["x1"] for d in g) + 3 * H
            if c["cx"] < gx0 or c["cx"] > gx1:
                continue
            dist = abs(c["cy"] - (a * c["cx"] + b0))
            if dist < bd:
                bd, best = dist, k
        if best >= 0 and bd < 0.9 * H:
            lines[best].append(c)
    out = []
    for g, (a, b0) in sorted(zip(lines, fits), key=lambda t: t[1][1] + t[1][0] * w / 2):
        if sum(c["area"] for c in g) < 4 * H:
            continue
        x0 = min(c["x0"] for c in g); x1 = max(c["x1"] for c in g)
        # EVERY PIECE IS MOVED UP OR DOWN SO THE LINE COMES OUT LEVEL
        shifts = [int(round(a * (c["cx"] - x0))) for c in g]
        top = min(c["y0"] - s for c, s in zip(g, shifts)); bot = max(c["y1"] - s for c, s in zip(g, shifts))
        pad = max(2, int(0.15 * H))
        canvas = np.zeros((bot - top + 2 * pad, x1 - x0 + 2 * pad), np.float32)
        for c, s in zip(g, shifts):
            sl = (slice(c["y0"], c["y1"]), slice(c["x0"], c["x1"]))
            m = (lab[sl] == c["id"])
            piece = ink[sl] * m
            yy = c["y0"] - s - top + pad; xx = c["x0"] - x0 + pad
            dst = canvas[yy:yy + piece.shape[0], xx:xx + piece.shape[1]]
            np.maximum(dst, piece, out=dst)
        a8 = np.clip((1.0 - canvas) * 255, 0, 255).astype(np.uint8)
        im = Image.fromarray(a8)
        im = im.resize((max(8, int(round(im.width * 32.0 / im.height))), 32), Image.BILINEAR)
        out.append(((x0, top, x1, bot), np.asarray(im)))
    return out


# ---------------------------------------------------------------- LINES BY STRIPS
def components(ink, b=0.35):
    from scipy import ndimage
    h, w = ink.shape
    lab, n = ndimage.label(ink > b, structure=np.ones((3, 3)))
    comps = []
    for k, sl in enumerate(ndimage.find_objects(lab)):
        if sl is None:
            continue
        y0, y1, x0, x1 = sl[0].start, sl[0].stop, sl[1].start, sl[1].stop
        m = lab[sl] == k + 1
        area = int(m.sum())
        if area < 6:
            continue
        if (x0 == 0 or x1 == w or y0 == 0 or y1 == h) and (y1 - y0 > h / 4 or x1 - x0 > w / 3):
            continue
        ys, xs = np.nonzero(m)
        comps.append(dict(id=k + 1, x0=x0, x1=x1, y0=y0, y1=y1, cx=x0 + xs.mean(), cy=y0 + ys.mean(),
                          h=y1 - y0, area=area))
    return lab, comps


def line_crops_strips(gray, debug=None):
    ink = erase_rules(ink_map(gray))
    h, w = ink.shape
    lab, comps = components(ink)
    keep = np.zeros_like(ink)
    for c in comps:
        sl = (slice(c["y0"], c["y1"]), slice(c["x0"], c["x1"]))
        keep[sl] = np.maximum(keep[sl], ink[sl] * (lab[sl] == c["id"]))
    hs = sorted(c["h"] for c in comps if c["area"] > 20)
    H = hs[len(hs) // 2] if hs else 10
    xs_ink = np.nonzero(keep.sum(0) > 0)[0]
    L, R = xs_ink.min(), xs_ink.max() + 1
    S = max(3, int((R - L) / (8 * H)))
    edges = np.linspace(L, R, S + 1).astype(int)
    from scipy.ndimage import gaussian_filter1d
    peaks = []
    for s in range(S):
        prof = gaussian_filter1d(keep[:, edges[s]:edges[s + 1]].sum(1), max(1.0, H / 3))
        top = prof.max()
        pk = [y for y in range(1, h - 1) if prof[y] >= prof[y - 1] and prof[y] > prof[y + 1] and prof[y] > 0.12 * top]
        # TOO CLOSE: KEEP THE STRONGER
        pk2 = []
        for y in pk:
            if pk2 and y - pk2[-1] < 0.7 * H:
                if prof[y] > prof[pk2[-1]]:
                    pk2[-1] = y
            else:
                pk2.append(y)
        peaks.append(pk2)
    diffs = [b - a for pk in peaks for a, b in zip(pk, pk[1:])]
    spacing = float(np.median(diffs)) if diffs else 2.0 * H
    # PATHS: A PEAK IN ONE STRIP CONTINUES TO THE NEAREST PEAK IN THE NEXT ONE
    paths = []
    open_paths = []
    for s in range(S):
        cx = (edges[s] + edges[s + 1]) / 2.0
        used = set()
        new_open = []
        cand = []
        for pi, p in enumerate(open_paths):
            last_x, last_y = p[-1]
            for qi, y in enumerate(peaks[s]):
                dy = abs(y - last_y)
                if dy < 0.45 * spacing:
                    cand.append((dy, pi, qi))
        cand.sort()
        taken_p = set()
        for dy, pi, qi in cand:
            if pi in taken_p or qi in used:
                continue
            taken_p.add(pi); used.add(qi)
            open_paths[pi].append((cx, peaks[s][qi]))
            new_open.append(open_paths[pi])
        for pi, p in enumerate(open_paths):
            if pi not in taken_p:
                paths.append(p)
        for qi, y in enumerate(peaks[s]):
            if qi not in used:
                new_open.append([(cx, y)])
        open_paths = new_open
    paths += open_paths

    def path_y(p, x):
        if len(p) == 1 or x <= p[0][0]:
            if len(p) >= 2 and x < p[0][0]:
                (x0, y0), (x1, y1) = p[0], p[1]
                return y0 + (y1 - y0) * (x - x0) / (x1 - x0) * 0.5
            return p[0][1]
        if x >= p[-1][0]:
            (x0, y0), (x1, y1) = p[-2], p[-1]
            return y1 + (y1 - y0) * (x - x1) / (x1 - x0) * 0.5
        for (x0, y0), (x1, y1) in zip(p, p[1:]):
            if x0 <= x <= x1:
                return y0 + (y1 - y0) * (x - x0) / (x1 - x0)
        return p[-1][1]

    def path_x_range(p):
        return p[0][0] - (edges[1] - edges[0]), p[-1][0] + (edges[1] - edges[0])
    # EVERY PIECE OF INK GOES TO THE PATH NEAREST TO ITS MIDDLE
    groups = [[] for _ in paths]
    for c in comps:
        best, bd = -1, 1e9
        for k, p in enumerate(paths):
            a, b = path_x_range(p)
            if c["cx"] < a or c["cx"] > b:
                continue
            d = abs(c["cy"] - path_y(p, c["cx"]))
            if d < bd:
                bd, best = d, k
        if best >= 0 and bd < 0.6 * spacing:
            groups[best].append(c)
    out = []
    order = sorted(range(len(paths)), key=lambda k: path_y(paths[k], w / 2))
    for k in order:
        g = groups[k]
        if not g or sum(c["area"] for c in g) < 3 * H:
            continue
        # ONLY LONG FLAT STROKES: AN UNDERLINE, NOT TEXT
        if all((c["x1"] - c["x0"]) > 3 * c["h"] and c["h"] < 0.8 * H for c in g if c["area"] > 10):
            continue
        p = paths[k]
        x0 = min(c["x0"] for c in g); x1 = max(c["x1"] for c in g)
        base = path_y(p, x0)
        shifts = [int(round(path_y(p, c["cx"]) - base)) for c in g]
        top = min(c["y0"] - s for c, s in zip(g, shifts)); bot = max(c["y1"] - s for c, s in zip(g, shifts))
        pad = max(2, int(0.15 * H))
        canvas = np.zeros((bot - top + 2 * pad, x1 - x0 + 2 * pad), np.float32)
        for c, s in zip(g, shifts):
            sl = (slice(c["y0"], c["y1"]), slice(c["x0"], c["x1"]))
            piece = ink[sl] * (lab[sl] == c["id"])
            yy = c["y0"] - s - top + pad; xx = c["x0"] - x0 + pad
            dst = canvas[yy:yy + piece.shape[0], xx:xx + piece.shape[1]]
            np.maximum(dst, piece, out=dst)
        a8 = np.clip((1.0 - canvas) * 255, 0, 255).astype(np.uint8)
        im = Image.fromarray(a8)
        im = im.resize((max(8, int(round(im.width * 32.0 / im.height))), 32), Image.BILINEAR)
        out.append(((x0, top, x1, bot), np.asarray(im)))
    if debug is not None:
        debug.update(H=H, spacing=spacing, S=S, paths=paths)
    return out
