"""用新地表素材 + 新手设计合成地图预览，确认观感。"""
import math
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
TILES = os.path.abspath(os.path.join(HERE, '..', 'assets', 'tiles'))
T = 16
GW, GH = 20, 12

# 四条路线：上环 / 直通 / 下环 / 之字（之字与直通形成两处路口）
PATHS = [
    [(0, 6), (3, 6), (3, 1), (16, 1), (17, 3), (19, 6)],
    [(0, 6), (15, 6), (19, 6)],
    [(0, 6), (3, 6), (3, 10), (16, 10), (17, 8), (19, 6)],
    [(0, 6), (6, 6), (6, 2), (10, 2), (10, 9), (14, 9), (14, 3), (18, 3), (18, 6), (19, 6)],
]
ROAD_W = 0.6


def load(n):
    return Image.open(os.path.join(TILES, n + '.png')).convert('RGBA')


def dist_seg(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    L2 = dx * dx + dy * dy
    t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / L2))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))


def road_dist(x, y):
    return min(dist_seg(x, y, *p[i], *p[i + 1]) for p in PATHS for i in range(len(p) - 1))


def h2(x, y, salt=0):
    """确定性哈希（与 C++/JS 侧实现保持一致），用来选地表变体"""
    v = ((x * 0x9E3779B1) ^ (y * 0x85EBCA77) ^ (salt * 0xC2B2AE3D)) & 0xFFFFFFFF
    v = ((v ^ (v >> 15)) * 0x2545F491) & 0xFFFFFFFF
    v = ((v ^ (v >> 13)) * 0x27D4EB2F) & 0xFFFFFFFF
    v = (v ^ (v >> 16)) & 0xFFFFFFFF
    return v


GRASS = [load('grass1'), load('grass2'), load('grass3'), load('grass4')]
WEIGHT = [0, 0, 0, 1, 1, 2, 3]      # grass1 出现最多
SAND = load('sand')
TREES = [load('tree1'), load('tree2'), load('tree3')]
FLOWER = load('flower')
BUSH = load('bush')

mp = Image.new('RGBA', ((GW + 2) * T, (GH + 2) * T))
OFF = T     # 外圈留一格给树林

road_cells = set()
for y in range(GH):
    for x in range(GW):
        d = road_dist(x, y)
        px, py = (x + 1) * T, (y + 1) * T
        if d < ROAD_W:
            mp.paste(SAND, (px, py))
            road_cells.add((x, y))
        else:
            g = GRASS[WEIGHT[h2(x, y, 1) % len(WEIGHT)]]
            mp.paste(g, (px, py))

# 外圈树林：先铺草地，再把树叠上去（树的四角是透明的，不铺草会露底）
for cy in range(-1, GH + 1):
    for cx in range(-1, GW + 1):
        if 0 <= cx < GW and 0 <= cy < GH:
            continue
        px, py = (cx + 1) * T, (cy + 1) * T
        mp.paste(GRASS[h2(cx, cy, 4) % len(GRASS)], (px, py))
        t = TREES[h2(cx, cy, 2) % 3]
        mp.paste(t, (px, py), t)

# 草地上的点缀：花丛 / 灌木（避开路面）
for y in range(GH):
    for x in range(GW):
        if (x, y) in road_cells:
            continue
        h = h2(x, y, 3) % 31
        px, py = (x + 1) * T, (y + 1) * T
        if h == 0:
            mp.paste(FLOWER, (px, py), FLOWER)
        elif h == 3:
            mp.paste(BUSH, (px, py), BUSH)

mp.resize((mp.width * 3, mp.height * 3), Image.NEAREST).save('map_new.png')
print('saved map_new.png', mp.size, '| 路面格子 %d / %d' % (len(road_cells), GW * GH))
