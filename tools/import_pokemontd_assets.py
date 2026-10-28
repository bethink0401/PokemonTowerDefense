"""从 ElasticSea/PokemonTD 仓库导入可用素材。

用法：
    python tools/import_pokemontd_assets.py <解包出的 unprocessed 目录> [assets 输出目录]

默认输出到 ../assets，处理内容：
  1. 地图主题：summer / winter 两套 TD 图块（48x48）
     - 把其中的青绿草地按调色板重映射为工程既有的绿宝石草地绿
     - 预合成 16 种邻接掩码的道路图块 road_0000.png ~ road_1111.png（位序 N=1 E=2 S=4 W=8）
     - 另导出 tree / tree_sub / tree_top / entrance
  2. 特效：投射物、命中/爆发序列帧、电属性黄色调变体
  3. 图标：技能图标(27)、状态图标、宝石、UI 图标
  4. 精灵：creeps(85) / towers(76) 32x32 像素精灵库

关键点（踩过的坑）：
  - summer/winter 的草地是**青绿色**（b > r），树冠是**正绿**（b <= r），必须用 b 与 r 的关系区分，
    否则会把树冠也一起改色。
  - 图块里青色系是全局固定的 4 个颜色，所以按**全图集统一排名**映射是安全的；
    若按单块内部排名映射，不同图块的明度基准会不一致，拼起来会出现深浅色块。
  - 道路图块必须做成**只保留道路像素、草地透明**的叠加层，否则十字/丁字路口的
    第二层会用它自带的草地盖掉第一层的路。
"""
import os
import shutil
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

# ---------------------------------------------------------------- 调色板

# 工程既有草地主色（取自 assets/tiles/grass1.png）
EMERALD = [
    (43, 110, 38),    # 最暗：草地边缘
    (76, 153, 56),
    (110, 190, 78),   # 主色 #6EBE4E
    (131, 197, 98),
]


def lum(p):
    return 0.30 * p[0] + 0.60 * p[1] + 0.10 * p[2]


def is_teal_grass(p):
    """青绿草地：蓝>红 且 绿明显大于红。树冠是正绿（蓝<红），可据此排除。"""
    r, g, b = p[0], p[1], p[2]
    return b > r and g > r + 20


def is_road(p):
    """土路 / 石路：偏暖（绿 <= 红），而草地是绿 >> 红。

    注意：判据必须在**重上色之前的原始像素**上使用。
    重上色后的草地绿是 (131,197,98)，它的 r-b 也 > 25，
    若用「红明显大于蓝」这种朴素判据，重上色后的草地会被误判成道路。
    """
    r, g = p[0], p[1]
    return g <= r


def process(im, gmap):
    """返回 (重上色后的完整图块, 仅含路面亮部的叠加层)。

    叠加层只保留「亮部路面」（r >= 150），把图块自带的**深色描边**（131,121,90 一类）剔掉。
    原因：描边在图块里是"路与草的交界"，多块叠加时它会留在路中间，看起来像一圈圈的环。
    描边最后由 add_outline() 按合成后的形状统一补上。
    """
    full = []
    core = []
    for p in im.get_flattened_data():
        if p[3] == 0:
            full.append(p)
            core.append((0, 0, 0, 0))
            continue
        c = p[:3]
        if is_teal_grass(c):
            full.append(gmap.get(c, c) + (p[3],))
            core.append((0, 0, 0, 0))
        elif is_road(c):
            full.append(p)
            if c[0] >= 150:
                core.append((c[0], c[1], c[2], 255))
            else:
                core.append((0, 0, 0, 0))
        else:
            full.append(p)          # 树冠等其它颜色原样保留
            core.append((0, 0, 0, 0))
    a = Image.new('RGBA', im.size)
    a.putdata(full)
    b = Image.new('RGBA', im.size)
    b.putdata(core)
    return a, b


OUTLINE = (131, 121, 90)


def road_mask(im):
    """取出图块里"路"的范围（含自带描边），并做闭运算填掉抖动产生的针孔。

    只用来确认路面的**宽度与纹理位置**，道路形状另由 shape_for() 程序化生成。
    """
    m = Image.new('L', im.size, 0)
    src = im.get_flattened_data()
    w, h = im.size
    px = m.load()
    i = 0
    for y in range(h):
        for x in range(w):
            p = src[i]
            i += 1
            if p[3] > 0 and is_road(p):
                px[x, y] = 255
    return m.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.MinFilter(3))


#: 路面宽度（48px 图块下原版道路约 24px 宽，取 22 略收一点，转角更利落）
BAND = 22


def shape_for(mask, size):
    """按邻接掩码程序化生成道路形状。

    为什么不用图块自带的形状：图块的道路边缘是"抖动 + 深色描边"的写法，
    取掩码会留下小孔、叠加时描边还会留在路中间（看起来像一圈圈的环）。
    这里改成"从中心向每个连通方向画一条粗线 + 中心圆盘"，
    所有掩码共用同一张纹理，因此任意叠加都无缝。
    """
    im = Image.new('L', (size, size), 0)
    if mask == 0:
        return im                      # 无连接 = 纯草地，中心圆盘不能画
    d = ImageDraw.Draw(im)
    c = (size - 1) / 2.0
    w = BAND
    mids = {'N': (c, 0), 'E': (size - 1, c), 'S': (c, size - 1), 'W': (0, c)}
    for name, bit in DIRS:
        if mask & bit:
            ex, ey = mids[name]
            d.line([c, c, ex, ey], fill=255, width=w)
    d.ellipse([c - w / 2, c - w / 2, c + w / 2, c + w / 2], fill=255)
    return im


def road_texture(im):
    """从一条直路图块的**内部**裁出正方形路面纹理，镜像平铺成正方形（保证无缝）。"""
    w, h = im.size
    px = im.load()
    rows = [y for y in range(h) if all(is_road(px[x, y]) for x in range(0, w, 4))]
    top, bot = rows[0], rows[-1] + 1
    side = max(8, min(bot - top - 6, 20))
    cx, cy = w // 2, (top + bot) // 2
    patch = im.crop((cx - side // 2, cy - side // 2,
                     cx - side // 2 + side, cy - side // 2 + side)).convert('RGBA')
    mir = Image.new('RGBA', (side * 2, side * 2))
    mir.paste(patch, (0, 0))
    mir.paste(patch.transpose(Image.FLIP_LEFT_RIGHT), (side, 0))
    mir.paste(patch.transpose(Image.FLIP_TOP_BOTTOM), (0, side))
    mir.paste(patch.transpose(Image.ROTATE_180), (side, side))
    return mir.resize((w, h), Image.NEAREST)


def stroke_outline(tile, shape, color=OUTLINE, thick=2):
    """按形状掩码给路面描深色边（只描一次，避免叠加层之间出现内部接缝）。"""
    w, h = tile.size
    ring = shape
    cur = shape
    for _ in range(thick):
        cur = cur.filter(ImageFilter.MinFilter(3))
        ring = ImageChops.subtract(ring, cur)
    tile.paste(Image.new('RGBA', (w, h), color + (255,)), (0, 0), ring)
    return tile


def build_grass_map(dirs):
    """扫描整个图集，收集所有青绿草地色，按明度排名映射到 EMERALD。"""
    colors = set()
    for d in dirs:
        for f in os.listdir(d):
            if not f.lower().endswith('.png'):
                continue
            im = Image.open(os.path.join(d, f)).convert('RGBA')
            for p in im.get_flattened_data():
                if p[3] > 0 and is_teal_grass(p):
                    colors.add(p[:3])
    colors = sorted(colors, key=lum)
    m = {}
    for i, c in enumerate(colors):
        idx = 0 if len(colors) == 1 else round(i * (len(EMERALD) - 1) / (len(colors) - 1))
        m[c] = EMERALD[idx]
    return m, colors


# ---------------------------------------------------------------- 道路合成

# 邻接掩码位序：N=1 E=2 S=4 W=8
DIRS = (('N', 1), ('E', 2), ('S', 4), ('W', 8))


# ---------------------------------------------------------------- 主流程

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    src = sys.argv[1]
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(root, 'assets')

    def mk(*p):
        d = os.path.join(out, *p)
        os.makedirs(d, exist_ok=True)
        return d

    tex = os.path.join(src, 'textures')

    # ---------- 1. 地图主题 ----------
    for theme in ('summer', 'winter'):
        srcdir = os.path.join(tex, 'map', theme)
        gmap, teal = build_grass_map([srcdir])
        print('[%s] 青绿草色 %d 种 -> 绿宝石草地绿' % (theme, len(teal)))

        dst = mk('tiles', theme)
        # 草地基色 + 统一的路面纹理 + 各方向的道路形状掩码
        grass = None
        tex_src = None
        for n, key in (('grass.png', 'grass'), ('pathHorizontal.png', 'h'),
                       ('pathVertical.png', 'v'), ('pathRound0.png', 'nw'),
                       ('pathRound1.png', 'ne'), ('pathRound2.png', 'es'),
                       ('pathRound3.png', 'sw')):
            orig = Image.open(os.path.join(srcdir, n)).convert('RGBA')
            full, _ = process(orig, gmap)
            if key == 'grass':
                full.save(os.path.join(dst, 'grass.png'))
                grass = full
            else:
                if tex_src is None:
                    tex_src = full

        texture = road_texture(tex_src)

        # 预合成 16 种邻接掩码：程序化形状 -> 填统一纹理 -> 统一描边
        for mask in range(16):
            shape = shape_for(mask, grass.size[0])
            tile = grass.copy()
            tile.paste(texture, (0, 0), shape)
            tile = stroke_outline(tile, shape)
            tile.save(os.path.join(dst, 'road_%d.png' % mask))
        print('   road_0..15.png 已合成（邻接掩码 N=1 E=2 S=4 W=8）')

        # 装饰 / 入口
        for n, key in (('tree.png', 'tree'), ('treeSub.png', 'tree_sub'),
                       ('treeTop.png', 'tree_top'), ('entrance.png', 'entrance')):
            p = os.path.join(srcdir, n)
            if os.path.exists(p):
                full, _ = process(Image.open(p).convert('RGBA'), gmap)
                full.save(os.path.join(dst, '%s.png' % key))
        print('   tree / tree_sub / tree_top / entrance 已导出')

    # ---------- 2. 特效 ----------
    fx = mk('fx')
    fs = mk('fx', 'seq')

    def cp(srcpath, dstdir, name=None):
        if not os.path.exists(srcpath):
            print('   ! 缺失', srcpath)
            return False
        shutil.copyfile(srcpath, os.path.join(dstdir, name or os.path.basename(srcpath)))
        return True

    # 投射物 / 命中贴图
    for n in ('fire.png', 'ice.png', 'leaf.png', 'rock.png', 'bullet.png', 'waterdrop.png',
              'bubble.png', 'sunbeam.png', 'poisonEffect.png', 'rockEffect.png'):
        cp(os.path.join(tex, n), fx)

    # 序列帧
    for pref, cnt in (('fireAnimation_', 10), ('fireball_', 5), ('orb_', 13),
                      ('charge_', 5), ('entangle_', 2)):
        key = {'fireAnimation_': 'fire'}.get(pref, pref.rstrip('_').lower())
        d = mk('fx', 'seq', key)
        for i in range(cnt):
            cp(os.path.join(tex, '%s%02d.png' % (pref, i)), d, '%02d.png' % i)

    # 电属性：把绿色 orb / 蓝色 charge 统一调成黄色（保明暗结构）
    for key in ('orb', 'charge'):
        s = os.path.join(fs, key)
        d = mk('fx', 'seq', 'electric_' + key)
        for f in sorted(os.listdir(s)):
            im = Image.open(os.path.join(s, f)).convert('RGBA')
            res = []
            for p in im.get_flattened_data():
                if p[3] == 0:
                    res.append(p)
                    continue
                L = lum(p[:3])
                res.append((min(255, int(L * 1.12)), min(255, int(L * 0.95)),
                            min(255, int(L * 0.26)), p[3]))
            o = Image.new('RGBA', im.size)
            o.putdata(res)
            o.save(os.path.join(d, f))
    print('fx: 投射物/序列帧 + 电属性黄色变体 已导出')

    # 金币 / 状态 / 提示类
    fui = mk('fx', 'ui')
    for n in ('coin.png', 'coinAttack.png', 'damage.png', 'hearth.png', 'speed.png',
              'blocked.png', 'maxed.png', 'grave.png', 'towernotification.png',
              'lifesteal.png', 'poison.png', 'slime.png', 'hypno.png', 'soundwave.png'):
        cp(os.path.join(tex, n), fui)

    # ---------- 3. 图标 ----------
    ai = mk('icons', 'ability')
    for f in sorted(os.listdir(os.path.join(tex, 'abilities'))):
        cp(os.path.join(tex, 'abilities', f), ai)
    si = mk('icons', 'status')
    for f in sorted(os.listdir(os.path.join(tex, 'effects'))):
        cp(os.path.join(tex, 'effects', f), si)
    # 状态图标补充（元素 / 异常）
    for n, dst_name in (('speed.png', 'slow.png'), ('poison.png', 'poison.png'),
                        ('lifesteal.png', 'lifesteal.png'), ('blocked.png', 'block.png')):
        cp(os.path.join(tex, n), si, dst_name)

    gi = mk('icons', 'gem')
    for f in sorted(os.listdir(os.path.join(tex, 'gems'))):
        cp(os.path.join(tex, 'gems', f), gi)

    ui = mk('icons', 'ui')
    for n, dst_name in (('shop.png', 'shop.png'), ('money.png', 'money.png'),
                        ('gold.png', 'gold.png'), ('treasure.png', 'treasure.png')):
        cp(os.path.join(tex, 'towers', n), ui, dst_name)
    for n, dst_name in (('summeron.png', 'season_summer_on.png'),
                        ('summeroff.png', 'season_summer_off.png'),
                        ('winteron.png', 'season_winter_on.png'),
                        ('winteroff.png', 'season_winter_off.png')):
        cp(os.path.join(tex, 'towers', n), ui, dst_name)
    print('icons: ability(%d) / status / gem / ui 已导出' % len(os.listdir(ai)))

    # ---------- 4. 精灵库 ----------
    cr = mk('sprites', 'creeps')
    for f in sorted(os.listdir(os.path.join(tex, 'creeps'))):
        cp(os.path.join(tex, 'creeps', f), cr)
    tw = mk('sprites', 'towers')
    for f in sorted(os.listdir(os.path.join(tex, 'towers'))):
        cp(os.path.join(tex, 'towers', f), tw)
    print('sprites: creeps(%d) / towers(%d) 已导出'
          % (len(os.listdir(cr)), len(os.listdir(tw))))

    # ---------- 5. 地图装饰 ----------
    dec = mk('tiles', 'decor')
    # 巨石（archive/rock.png）
    arch = os.path.join(src, 'archive')
    cp(os.path.join(arch, 'rock.png'), dec, 'rock.png')
    cp(os.path.join(arch, 'gate.png'), dec, 'gate.png')
    cp(os.path.join(arch, 'gateBase.png'), dec, 'gate_base.png')
    # 从 tileset.png 裁出灌木与花丛
    tsf = os.path.join(arch, 'images', 'tileset.png')
    if os.path.exists(tsf):
        im = Image.open(tsf).convert('RGBA')
        w, h = im.size
        im.crop((int(w * 0.55), int(h * 0.80), w, h)).save(os.path.join(dec, 'bush.png'))
        im.crop((int(w * 0.60), 0, int(w * 0.90), int(h * 0.09))).save(
            os.path.join(dec, 'flowers.png'))
        print('decor: rock / gate / bush / flowers 已导出')
    return 0


if __name__ == '__main__':
    sys.exit(main())
