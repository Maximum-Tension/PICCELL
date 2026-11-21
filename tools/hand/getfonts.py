import json, urllib.request, re, os, concurrent.futures as cf
u = json.load(open('fonturls.json'))
os.makedirs('fonts', exist_ok=True); os.makedirs('fonts_print', exist_ok=True); os.makedirs('meta', exist_ok=True)
def get(j):
    d, n, url = j
    if os.path.exists(os.path.join(d, n)): return n
    open(os.path.join(d, n), 'wb').write(urllib.request.urlopen(url, timeout=60).read()); return n
def getp(path):
    t = urllib.request.urlopen('https://raw.githubusercontent.com/google/fonts/main/' + path + '/METADATA.pb', timeout=30).read().decode()
    files = re.findall(r'filename:\s*"([^"]+)"', t)
    pick = [f for f in files if 'Italic' not in f]
    f = ([x for x in pick if 'Regular' in x or '[' in x] or pick or files)[0]
    return get(('fonts_print', f.replace('[', '_').replace(']', '_'), 'https://raw.githubusercontent.com/google/fonts/main/%s/%s' % (path, urllib.request.quote(f))))
with cf.ThreadPoolExecutor(8) as ex:
    a = list(ex.map(get, [tuple(x) for x in u['hand']])); b = list(ex.map(getp, u['printed_families']))
print(len(a), len(b))
