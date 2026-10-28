"""下载渲染绿宝石地表图块所需的原始数据（从 pret/pokeemerald）。

用法：
    python tools/fetch_pokeemerald_assets.py [工作目录]

会在工作目录（默认 ./.tileset_work）下生成：
    tiles.png  metatiles.bin  pal_00.pal ~ pal_15.pal

之后在同一目录运行 make_terrain_tiles.py 即可生成 assets/tiles/ 下的地表素材。
"""
import os
import subprocess
import sys

BASE = "https://raw.githubusercontent.com/pret/pokeemerald/master/data/tilesets/primary/general"
work = sys.argv[1] if len(sys.argv) > 1 else ".tileset_work"
os.makedirs(work, exist_ok=True)


def get(url, name):
    out = os.path.join(work, name)
    subprocess.run(["curl", "-sSL", "-o", out, url], check=True)
    print("  %-12s %d bytes" % (name, os.path.getsize(out)))


print("downloading tileset data ->", work)
get(BASE + "/tiles.png", "tiles.png")
get(BASE + "/metatiles.bin", "metatiles.bin")
for i in range(16):
    get(BASE + "/palettes/%02d.pal" % i, "pal_%02d.pal" % i)
print("done. next: cd", work, "&& python ../tools/make_terrain_tiles.py")
