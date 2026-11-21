# A SMALL CONVOLUTIONAL LINE READER WITH CTC. INPUT: GRAYSCALE LINE, HEIGHT 32.
# EVERY OUTPUT COLUMN COVERS 4 PIXELS OF THE LINE. ONLY CONV, BATCHNORM (FOLDED
# INTO THE CONV WHEN EXPORTED), RELU AND MAX POOLING: EASY TO RUN IN PLAIN C.
import glob, os, sys, time, math, random
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from synth import CHARSET

torch.set_num_threads(2)
NCLASS = len(CHARSET) + 1  # 0 = BLANK
CHAR_ID = {c: i + 1 for i, c in enumerate(CHARSET)}


def block(i, o, k=3, p=1):
    return [nn.Conv2d(i, o, k, padding=p, bias=False), nn.BatchNorm2d(o), nn.ReLU(inplace=True)]


class Reader(nn.Module):
    def __init__(self, c=(32, 64, 128, 160, 192)):
        super().__init__()
        self.features = nn.Sequential(
            *block(1, c[0]), nn.MaxPool2d(2, 2),                 # 16 x W/2
            *block(c[0], c[1]), nn.MaxPool2d(2, 2),              # 8 x W/4
            *block(c[1], c[2]), *block(c[2], c[2]), nn.MaxPool2d((2, 1), (2, 1)),  # 4
            *block(c[2], c[3]), *block(c[3], c[3]), nn.MaxPool2d((2, 1), (2, 1)),  # 2
            nn.Conv2d(c[3], c[4], (2, 1), bias=False), nn.BatchNorm2d(c[4]), nn.ReLU(inplace=True),  # 1
        )
        # CONTEXT ALONG THE LINE: WIDE 1D CONVOLUTIONS (AS 2D WITH HEIGHT 1)
        self.context = nn.Sequential(
            nn.Conv2d(c[4], c[4], (1, 5), padding=(0, 2), bias=False), nn.BatchNorm2d(c[4]), nn.ReLU(inplace=True),
            nn.Conv2d(c[4], c[4], (1, 5), padding=(0, 2), bias=False), nn.BatchNorm2d(c[4]), nn.ReLU(inplace=True),
        )
        self.out = nn.Conv2d(c[4], NCLASS, 1)

    def forward(self, x):          # x: B x 1 x 32 x W, 0 = PAPER, 1 = INK
        f = self.features(x)
        f = f + self.context(f)
        return self.out(f).squeeze(2)  # B x C x W/4


def load_chunks(paths):
    imgs, texts = [], []
    for p in paths:
        z = np.load(p, allow_pickle=True)
        w, d, t = z["widths"], z["data"], z["texts"]
        pos = 0
        for k in range(len(w)):
            n = 32 * int(w[k])
            imgs.append(d[pos:pos + n].reshape(32, int(w[k])))
            pos += n
            texts.append(str(t[k]))
    return imgs, texts


def to_input(img):
    # INK = 1, PAPER = 0
    return (255.0 - img.astype(np.float32)) / 255.0


def make_batch(imgs, texts, idx):
    W = max(imgs[i].shape[1] for i in idx)
    W = (W + 3) // 4 * 4
    x = np.zeros((len(idx), 1, 32, W), np.float32)
    targets, lens, in_lens = [], [], []
    for b, i in enumerate(idx):
        a = to_input(imgs[i])
        x[b, 0, :, :a.shape[1]] = a
        ids = [CHAR_ID[c] for c in texts[i] if c in CHAR_ID]
        targets += ids
        lens.append(len(ids))
        in_lens.append(W // 4)
    return (torch.from_numpy(x), torch.tensor(targets, dtype=torch.long),
            torch.tensor(in_lens, dtype=torch.long), torch.tensor(lens, dtype=torch.long))


def greedy(logits):
    # logits: C x T
    best = logits.argmax(0).tolist()
    out, prev = [], 0
    for b in best:
        if b != prev and b != 0:
            out.append(CHARSET[b - 1])
        prev = b
    return "".join(out)


def edit(a, b):
    d = list(range(len(b) + 1))
    for i in range(1, len(a) + 1):
        p, d[0] = d[0], i
        for j in range(1, len(b) + 1):
            p, d[j] = d[j], min(d[j] + 1, d[j - 1] + 1, p + (a[i - 1] != b[j - 1]))
    return d[len(b)]


def evaluate(model, imgs, texts, n=600):
    model.eval()
    errs = chars = 0
    lines_ok = 0
    with torch.no_grad():
        order = sorted(range(min(n, len(imgs))), key=lambda i: imgs[i].shape[1])
        for s in range(0, len(order), 32):
            idx = order[s:s + 32]
            x, _, _, _ = make_batch(imgs, texts, idx)
            y = model(x)
            for b, i in enumerate(idx):
                w = imgs[i].shape[1] // 4 + 1
                got = greedy(y[b, :, :w])
                errs += edit(got, texts[i]); chars += len(texts[i])
                lines_ok += got == texts[i]
    model.train()
    return errs / max(1, chars), lines_ok / max(1, min(n, len(imgs)))


REAL = []
PAGE_FROM = [0]
LR = float(os.environ.get("LR", "2e-3"))


def load_real():
    import page
    from PIL import Image
    path = os.path.join(HERE, "real", "el_yazisi.png")
    truth = [l.rstrip("\n") for l in open(os.path.join(HERE, "real", "el_yazisi.txt"), encoding="utf-8")]
    crops = page.line_crops_strips(np.asarray(Image.open(path).convert("L")))
    # THE 18 LIST ROWS: CROPS 1..18 ARE THE ROWS (0 IS THE HEADER)
    for k in range(1, 19):
        REAL.append((crops[k][1], truth[k]))


def real_cer(model):
    import re
    def norm(s):
        s = re.sub(r"\s*-\s*", "-", s.replace("—", "-"))
        return "".join(s.split()).lower()
    model.eval()
    e = n = 0
    with torch.no_grad():
        for a, t in REAL:
            x = torch.from_numpy(to_input(a))[None, None]
            x = F.pad(x, (0, (4 - x.shape[3] % 4) % 4))
            g = greedy(model(x)[0])
            e += edit(norm(g), norm(t)); n += len(norm(t))
    model.train()
    return e / max(1, n)


def main():
    minutes = float(sys.argv[1]) if len(sys.argv) > 1 else 30
    resume = len(sys.argv) > 2 and sys.argv[2] == "resume"
    paths = sorted(glob.glob(os.path.join(HERE, "data", "chunk_*.npz"))) + sorted(glob.glob(os.path.join(HERE, "data", "pchunk_*.npz")))
    # CCHUNKS: PAGE CROPS CUT BY PICCELL'S OWN C CODE (GEN_CCHUNKS.PY)
    paths += sorted(glob.glob(os.path.join(HERE, "data", "cchunk_*.npz")))
    # OLD CHUNKS CAN BE LEFT OUT (MEMORY): TRAIN_SKIP="chunk_00,chunk_01"
    skip = [x for x in os.environ.get("TRAIN_SKIP", "").split(",") if x]
    paths = [p for p in paths if not any(os.path.basename(p).startswith(k) for k in skip)]
    test_paths = [p for p in paths if p.endswith("chunk_0100.npz")]
    train_paths = [p for p in paths if p not in test_paths]
    timgs, ttexts = load_chunks(test_paths)
    line_paths = [p for p in train_paths if "pchunk_" not in p and "cchunk_" not in p]
    page_paths = [p for p in train_paths if "pchunk_" in p or "cchunk_" in p]
    imgs, texts = load_chunks(line_paths)
    PAGE_FROM[0] = len(imgs)
    pi, pt = load_chunks(page_paths)
    imgs += pi; texts += pt
    print("train", len(imgs), "test", len(timgs), flush=True)
    load_real()
    model = Reader()
    print("params", sum(p.numel() for p in model.parameters()), flush=True)
    opt = torch.optim.AdamW(model.parameters(), lr=LR, weight_decay=1e-4)
    step = 0
    ck = os.path.join(HERE, "reader.pt")
    if resume and os.path.exists(ck):
        s = torch.load(ck)
        model.load_state_dict(s["model"]); step = s["step"]
        # AN AVERAGED MODEL (SWA.PY) HAS NO OPTIMIZER STATE: A FRESH ONE
        if "opt" in s:
            opt.load_state_dict(s["opt"])
        print("resumed at", step, flush=True)
    ctc = nn.CTCLoss(blank=0, zero_infinity=True)
    # BATCHES OF SIMILAR WIDTH: LESS PADDING. THE PAGE CROPS (WHAT THE PAGE
    # FINDER REALLY GIVES) COME THREE TIMES.
    page_from = PAGE_FROM[0]
    order = list(range(len(imgs))) + list(range(page_from, len(imgs))) * 2
    order = sorted(order, key=lambda i: imgs[i].shape[1])
    BATCH = 32
    batches = [order[i:i + BATCH] for i in range(0, len(order), BATCH)]
    start = time.time()
    total_steps_est = None
    while time.time() - start < minutes * 60:
        random.shuffle(batches)
        for idx in batches:
            if time.time() - start >= minutes * 60:
                break
            x, t, il, tl = make_batch(imgs, texts, idx)
            y = model(x)                          # B x C x T
            lp = F.log_softmax(y, 1).permute(2, 0, 1)  # T x B x C
            loss = ctc(lp, t, il, tl)
            opt.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 5.0)
            # COSINE-ISH DECAY OVER THE RUN
            frac = (time.time() - start) / (minutes * 60)
            for g in opt.param_groups:
                g["lr"] = LR * (0.05 + 0.95 * 0.5 * (1 + math.cos(math.pi * min(1, frac))))
            opt.step()
            step += 1
            if step % 200 == 0:
                print("step %d loss %.3f lr %.5f %.0fs" % (step, loss.item(), opt.param_groups[0]["lr"], time.time() - start), flush=True)
            if step % 1000 == 0:
                cer, ok = evaluate(model, timgs, ttexts, 400)
                print("  test CER %.3f lines %.3f" % (cer, ok), flush=True)
                if REAL:
                    print("  real CER %.3f" % real_cer(model), flush=True)
                torch.save({"model": model.state_dict(), "opt": opt.state_dict(), "step": step}, ck)
    cer, ok = evaluate(model, timgs, ttexts, 1000)
    print("final test CER %.3f lines %.3f steps %d" % (cer, ok, step), flush=True)
    torch.save({"model": model.state_dict(), "opt": opt.state_dict(), "step": step}, ck)


if __name__ == "__main__":
    main()
