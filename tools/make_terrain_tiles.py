"""生成最终地表素材：多种草地/树木变体，让地图不再单一重复。"""
import struct
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.abspath(os.path.join(HERE, '..', 'assets', 'tiles'))
os.makedirs(OUT, exist_ok=True)

TILES, META = 'tiles.png', 'metatiles.bin'
T = 16
GRASS_RAMP = [(57, 139, 49), (110, 190, 78), (180, 255, 131)]

# 选中图块（metatile 索引）：4 种草地 + 沙路 + 3 种树 + 花丛 + 灌木
CHOSEN = {
    # 四种草地图块全部选"低渐变"的，否则平铺会出现竖条纹
    # （实测 m70/m13 的左右/上下亮度差高达 26 / 25，会形成明显条纹）
    'grass1': (1, True),     # 平整，3 色
    'grass2': (28, True),    # 平整，斑点位置不同
    'grass3': (473, True),   # 平整，4 色
    'grass4': (462, True),   # 轻微纹理（grad 4.1）
    'sand':   (435, False),  # 土路
    'tree1':  (198, False),  # 树篱（平整，可无缝拼接）
    'tree2':  (199, False),
    'tree3':  (64, False),   # 圆形树冠
    'flower': (4, True),     # 花丛
    'bush':   (64, True),    # 小灌木
}
# 需要把"青绿底色"抹成透明的装饰类图块
DECOR = {'flower', 'bush'}


def load_pal(path):
    lines = [l.strip() for l in open(path) if l.strip()]
    cols = []
    for l in lines[3:3 + int(lines[2])]:
        r, g, b = [int(x) for x in l.split()[:3]]
        cols.append((r, g, b, 0 if len(cols) == 0 else 255))
    while len(cols) < 16:
        cols.append((0, 0, 0, 0))
    cols[0] = (cols[0][0], cols[0][1], cols[0][2], 0)
    return cols


pals = [load_pal('pal_%02d.pal' % i) for i in range(16)]
sheet = Image.open(TILES)
idx_all = list(sheet.getdata())
SW, SH = sheet.size
TPR = SW // 8
raw = open(META, 'rb').read()


def render(m):
    out = Image.new('RGBA', (T, T), (0, 0, 0, 0))
    for layer in (0, 1):
        for k in range(4):
            v = struct.unpack_from('<H', raw, m * 16 + layer * 8 + k * 2)[0]
            tile = v & 0x3FF
            xf, yf, pal = (v >> 10) & 1, (v >> 11) & 1, (v >> 12) & 0xF
            if tile >= TPR * (SH // 8):
                continue
            sx, sy = (tile % TPR) * 8, (tile // TPR) * 8
            cols = pals[pal]
            for yy in range(8):
                for xx in range(8):
                    c = cols[idx_all[(sy + yy) * SW + (sx + xx)] % 16]
                    if c[3] == 0:
                        continue
                    dx = xx if not xf else 7 - xx
                    dy = yy if not yf else 7 - yf
                    out.putpixel(((k % 2) * 8 + dx, (k // 2) * 8 + dy), c)
    return out


def to_grass(im):
    """把青绿色系重映射为绿宝石草地绿。

    关键：把每块图块的**主色**统一映射到同一个基准绿，只按"比主色亮/暗"决定
    高光与阴影。否则各图块原本的主色深浅不同，拼在一起会出现明显的深浅色块。
    """
    from collections import Counter
    data = list(im.getdata())
    teals = [p[:3] for p in data if p[3] > 0 and is_teal(p[:3])]
    if not teals:
        return im
    base = Counter(teals).most_common(1)[0][0]
    bs = sum(base) / 3.0

    res = []
    for p in data:
        if p[3] == 0:
            res.append(p)
            continue
        c = p[:3]
        if is_teal(c):
            s = sum(c) / 3.0
            if abs(s - bs) < 12:
                idx = 1                       # 基准绿
            elif s > bs:
                idx = 2                       # 高光
            else:
                idx = 0                       # 阴影
            res.append(GRASS_RAMP[idx] + (255,))
        else:
            res.append(p)
    out = Image.new('RGBA', im.size)
    out.putdata(res)
    return out


def is_teal(c):
    r, g, b = c
    return g > r + 18 and b > r + 8


def teal_transparent(im):
    """把青绿底色抹成透明，让草地透出来（装饰类图块用）。"""
    data = list(im.getdata())
    out = Image.new('RGBA', im.size)
    out.putdata([(p[0], p[1], p[2], 0) if (p[3] > 0 and is_teal(p[:3])) else p for p in data])
    return out


for name, (m, is_grass) in CHOSEN.items():
    im = render(m)
    if is_grass:
        im = to_grass(im)
    if name in DECOR:
        im = teal_transparent(im)
    im.save(os.path.join(OUT, name + '.png'))
    print('%-8s <- metatile %-4d %s%s' % (name, m,
                                          'grass-recolored ' if is_grass else 'original ',
                                          'bg-cleared' if name in DECOR else ''))

# 清掉不再使用的旧素材（注意不要删掉本次生成的文件）
for old in ('grass.png', 'tree.png', 'water.png', '_map_preview.png'):
    p = os.path.join(OUT, old)
    if os.path.exists(p):
        os.remove(p)
        print('removed old', old)
print('done ->', OUT)
