# THE TRAINED READER TO READER.BIN FOR PICCELL.EXE (SEE SRC/READER.H)
import os, sys, struct
import numpy as np
import torch
import torch.nn as nn

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from synth import CHARSET
import train

STEP_CONV, STEP_POOL, STEP_KEEP, STEP_ADD = 1, 2, 3, 4


def fold(conv, bn):
    w = conv.weight.detach().double().numpy()
    b = conv.bias.detach().double().numpy() if conv.bias is not None else np.zeros(w.shape[0])
    if bn is not None:
        g = bn.weight.detach().double().numpy(); beta = bn.bias.detach().double().numpy()
        mean = bn.running_mean.double().numpy(); var = bn.running_var.double().numpy()
        s = g / np.sqrt(var + bn.eps)
        w = w * s[:, None, None, None]
        b = (b - mean) * s + beta
    return w, b


def conv_step(conv, bn, relu):
    w, b = fold(conv, bn)
    o, i, kh, kw = w.shape
    ph, pw = conv.padding
    out = struct.pack("<8i", STEP_CONV, i, o, kh, kw, ph, pw, 1 if relu else 0)
    flat = w.reshape(o, -1)
    scale = np.abs(flat).max(1) / 127.0
    scale[scale == 0] = 1e-8
    q = np.clip(np.round(flat / scale[:, None]), -127, 127).astype(np.int8)
    out += scale.astype("<f4").tobytes() + q.tobytes() + b.astype("<f4").tobytes()
    return out


def walk(seq):
    # nn.Sequential OF CONV / BN / RELU / MAXPOOL -> STEPS
    steps = []
    mods = list(seq)
    k = 0
    while k < len(mods):
        m = mods[k]
        if isinstance(m, nn.Conv2d):
            bn = mods[k + 1] if k + 1 < len(mods) and isinstance(mods[k + 1], nn.BatchNorm2d) else None
            j = k + 1 + (bn is not None)
            relu = j < len(mods) and isinstance(mods[j], nn.ReLU)
            steps.append(conv_step(m, bn, relu))
            k = j + (1 if relu else 0)
        elif isinstance(m, nn.MaxPool2d):
            ks = m.kernel_size if isinstance(m.kernel_size, tuple) else (m.kernel_size, m.kernel_size)
            steps.append(struct.pack("<3i", STEP_POOL, ks[0], ks[1]))
            k += 1
        else:
            raise SystemExit("unknown module %r" % m)
    return steps


def export(model, path):
    model.eval()
    steps = walk(model.features)
    steps.append(struct.pack("<2i", STEP_KEEP, 0))
    steps += walk(model.context)
    steps.append(struct.pack("<2i", STEP_ADD, 0))
    steps.append(conv_step(model.out, None, False))
    chars = CHARSET.encode("utf-8")
    blob = b"TRD1" + struct.pack("<2i", train.NCLASS, 32) + struct.pack("<i", len(chars)) + chars
    blob += struct.pack("<i", len(steps)) + b"".join(steps)
    open(path, "wb").write(blob)
    return len(blob)


if __name__ == "__main__":
    ck = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "reader.pt")
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, "reader.bin")
    m = train.Reader()
    m.load_state_dict(torch.load(ck)["model"])
    print("wrote", out, export(m, out), "bytes")
