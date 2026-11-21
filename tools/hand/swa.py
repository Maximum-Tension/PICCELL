# THE AVERAGE OF SOME CHECKPOINTS (SWA), WITH THE BATCHNORM NUMBERS MEASURED
# AGAIN ON TRAINING LINES: SWA.PY OUT.PT CK_A.PT CK_B.PT ...
import glob, os, random, sys
import torch
import torch.nn as nn

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import train


def main():
    out, cks = sys.argv[1], sys.argv[2:]
    states = [torch.load(c, map_location="cpu")["model"] for c in cks]
    avg = {}
    for k in states[0]:
        if states[0][k].dtype.is_floating_point:
            avg[k] = sum(s[k] for s in states) / len(states)
        else:
            avg[k] = states[-1][k]
    model = train.Reader()
    model.load_state_dict(avg)
    # BATCHNORM: A PLAIN AVERAGE OVER THE BATCHES BELOW
    for m in model.modules():
        if isinstance(m, nn.BatchNorm2d):
            m.reset_running_stats()
            m.momentum = None
    data = os.path.join(HERE, "data")
    paths = sorted(glob.glob(os.path.join(data, "chunk_02*.npz")))[-3:]
    paths += sorted(glob.glob(os.path.join(data, "pchunk_*.npz")))[-2:]
    imgs, texts = train.load_chunks(paths)
    order = sorted(range(len(imgs)), key=lambda i: imgs[i].shape[1])
    batches = [order[i:i + 32] for i in range(0, len(order), 32)]
    random.seed(1)
    random.shuffle(batches)
    model.train()
    with torch.no_grad():
        for idx in batches[:300]:
            x, _, _, _ = train.make_batch(imgs, texts, idx)
            model(x)
    model.eval()
    torch.save({"model": model.state_dict(), "step": 0}, out)
    print("saved", out, len(batches[:300]), "batches")


if __name__ == "__main__":
    main()
