"""扫描低渐变的平整草地图块（避免平铺出现条纹）。"""
import struct
from PIL import Image

T = 16


def load_pal(p):
    L = [l.strip() for l in open(p) if l.strip()]
    c = []
    for l in L[3:3 + int(L[2])]:
        r, g, b = [int(x) for x in l.split()[:3]]
        c.append((r, g, b, 0 if len(c) == 0 else 255))
    while len(c) < 16:
        c.append((0, 0, 0, 0))
    c[0] = (c[0][0], c[0][1], c[0][2], 0)
    return c


pals = [load_pal('pal_%02d.pal' % i) for i in range(16)]
sh = Image.open('tiles.png')
ia = list(sh.getdata())
SW, SH = sh.size
TPR = SW // 8
raw = open('metatiles.bin', 'rb').read()
N = len(raw) // 16


def render(m):
    o = Image.new('RGBA', (T, T), (0, 0, 0, 0))
    for layer in (0, 1):
        for k in range(4):
            v = struct.unpack_from('<H', raw, m * 16 + layer * 8 + k * 2)[0]
            ti, xf, yf, pl = v & 0x3FF, (v >> 10) & 1, (v >> 11) & 1, (v >> 12) & 0xF
            if ti >= TPR * (SH // 8):
                continue
            sx, sy = (ti % TPR) * 8, (ti // TPR) * 8
            cols = pals[pl]
            for yy in range(8):
                for xx in range(8):
                    c = cols[ia[(sy + yy) * SW + (sx + xx)] % 16]
                    if c[3] == 0:
                        continue
                    dx = xx if not xf else 7 - xx
                    dy = yy if not yf else 7 - yy
                    o.putpixel(((k % 2) * 8 + dx, (k // 2) * 8 + dy), c)
    return o


rows = []
for m in range(N):
    im = render(m)
    px = list(im.getdata())
    op = [p for p in px if p[3] > 0]
    if len(op) < 250:
        continue
    gc = sum(1 for r, g, b, a in op if g > r + 18 and g > b + 8) / len(op)
    if gc < 0.93:
        continue
    u = len(set(op))
    if u > 8:
        continue
    lum = [0.3 * p[0] + 0.6 * p[1] + 0.1 * p[2] for p in px]

    def avg(f):
        v = [lum[y * T + x] for y in range(T) for x in range(T) if f(x, y)]
        return sum(v) / max(1, len(v))

    L, R = avg(lambda x, y: x < 8), avg(lambda x, y: x >= 8)
    Tp, B = avg(lambda x, y: y < 8), avg(lambda x, y: y >= 8)
    grad = max(abs(L - R), abs(Tp - B))
    rows.append((grad, u, m, round(gc, 2)))

rows.sort()
print('%-7s %-5s %-5s %s' % ('grad', 'uniq', 'meta', 'greenfrac'))
for r in rows[:18]:
    print('%-7.1f %-5d %-5d %s' % r)
