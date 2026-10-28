# 素材来源与说明（assets/）

本目录中的图片均为**宝可梦原生（官方/游戏原版）素材**，仅用于课程作业演示。

## 0. 战场地图 `maps/`

| 文件 | 说明 |
|---|---|
| `forest_route.png` / `.jpg` | 当前战场整图（像素风森林路线，1446×1087，25×19 逻辑格） |
| `_terrain_analysis.png` | 地形分析校验图：橙色=道路、绿色=高草丛(可部署)、蓝=水，含 BFS 路径与刷怪点标注 |

来源：用户提供的像素风地图图。地形网格由 `tools/analyze_forest_map.py`
自动分析生成（`core/ForestMapData.h` 与预览页 TERRAIN 常量同源）。

## 1. 宝可梦贴图 `sprites/pokemon/`

来源：**PokéAPI sprites（官方游戏像素图，96×96）**——即《宝可梦》正作游戏内
实际使用的点阵贴图，按图鉴号归档于
`PokeAPI/sprites` 仓库的 `sprites/pokemon/<图鉴号>.png`
（另有 `versions/generation-v/black-white/` 第五世代黑白版，两套基本一致，本工程用前者）。

| 文件 | 宝可梦 | 图鉴号 | 用途 |
|---|---|---|---|
| `pichu.png` | 皮丘 | 172 | 玩家单位（电）→ 进化链 Pichu→Pikachu→Raichu |
| `pikachu.png` | 皮卡丘 | 25 | 进化形态（阶段 1） |
| `raichu.png` | 雷丘 | 26 | 进化形态（阶段 2，最终） |
| `charmander.png` | 小火龙 | 4 | 玩家单位（火）→ Charmeleon(5) → Charizard(6) |
| `charmeleon.png` / `charizard.png` | 火恐龙 / 喷火龙 | 5 / 6 | 进化形态 |
| `squirtle.png` | 杰尼龟 | 7 | 玩家单位（水）→ Wartortle(8) → Blastoise(9) |
| `wartortle.png` / `blastoise.png` | 卡咪龟 / 水箭龟 | 8 / 9 | 进化形态 |
| `bulbasaur.png` | 妙蛙种子 | 1 | 玩家单位（草）→ Ivysaur(2) → Venusaur(3) |
| `ivysaur.png` / `venusaur.png` | 妙蛙草 / 妙蛙花 | 2 / 3 | 进化形态 |
| `enemy_basic.png` | 走路草 | 43 | 敌人 BasicEnemy（草） |
| `enemy_fast.png` | 霹雳电球 | 100 | 敌人 FastEnemy（电，高速） |
| `enemy_tank.png` | 拉普拉斯 | 131 | 敌人 TankEnemy（水，高肉度） |

> **渲染要点**：96×96 像素图放大时必须用**最近邻插值**才能保住颗粒感——
> 预览页是 `ctx.imageSmoothingEnabled = false`，Qt 侧所有精灵缩放改为
> `Qt::FastTransformation`（属性徽章这类小图标不受影响）。
> 顺带的好处：整包素材从 ~880KB 降到 ~130KB，预览页从 1.2MB 降到 265KB。

## 2. 属性图标 `sprites/types/`

来源：PokéAPI sprites 仓库（第八世代属性徽章）
`sprites/types/generation-viii/sword-shield/<属性编号>.png`

Fire=10、Water=11、Grass=12、Electric=13、Ice=15。

## 3. 地图地面图块 `tiles/`

来源：**pokeemerald**（宝可梦绿宝石反编译工程，pret 组织）原始图块数据
https://github.com/pret/pokeemerald

- `data/tilesets/primary/general/tiles.png` —— 4bpp 索引图块数据
- `data/tilesets/primary/general/metatiles.bin` + `metatile_attributes.bin` —— metatile 组装数据
- `data/tilesets/primary/general/palettes/00..15.pal` —— 游戏原始 JASC-PAL 调色板

按 GBA BG 图块位序（0-9 位图块号、10/11 位翻转、12-15 位调色板号）+ 原始调色板
还原为真实上色的 metatile 图集后，筛选出以下地面图块：

| 文件 | 内容 | metatile 索引 |
|---|---|---|
| `grass1.png` ~ `grass4.png` | 四种草地变体（基准 / 斑点 / 浅色 / 细密） | 1 / 28 / 473 / 462 |
| `sand.png` | 沙地 / 土路 | 435 |
| `tree1.png` `tree2.png` | 树篱（可无缝拼接，用作外圈） | 198 / 199 |
| `tree3.png` | 圆形树冠 | 64 |
| `flower.png` | 花丛点缀（已抹掉自带底色） | 4 |
| `bush.png` | 小灌木点缀（已抹掉自带底色） | 64 |
| `_map_preview.png` | 用上述图块拼出的地图效果预览 | — |

> 草地处理说明：这些图块在绿宝石里是**青色调**（palette 3）。处理方式是——
> 1. 挑**低渐变**的图块：实测 `m70`/`m13` 的左右/上下亮度差高达 26/25，平铺会出现明显竖条纹；
>    最终选中的 4 张都 ≤ 4.1。
> 2. 把每块图块的**主色统一映射到同一基准绿**，只按"比主色亮/暗"决定高光与阴影，
>    否则各图块原本主色深浅不同，拼在一起会出现深浅色块。
> 3. 装饰类（花丛/灌木）把**青绿底色抹成透明**，让它自然落在草地上，而不是带着一块异色方底。
>
> 重映射用的是绿宝石自身调色板里的草地绿（#398B31 / #6EBE4E / #B4FF83），图块形状与像素分布完全保留原版。

## 4. 界面窗口边框 `ui/`

来源：同上的 `pret/pokeemerald` —— `graphics/text_window/3.png`（绿宝石里经典的花饰对话框边框，
24×24 = 3×3 个 8×8 图块）+ `text_pal1/2/4.pal`。

| 文件 | 内容 | 用法 |
|---|---|---|
| `window_white.png` | 白底 + 蓝/红花饰边框（`text_pal4`） | 普通面板（部署栏 / 信息栏） |
| `window_blue.png` | 蓝底菜单窗（`text_pal2`） | 状态栏 / 指令栏 / 战斗日志 / 地图外框 |
| `window_green.png` | 备选配色（`text_pal1`） | 备用 |

渲染要点：图块的**索引 0 是透明色**（即圆角外侧的镂空），中间 8×8 图块是**填充色**（索引 14）。
素材按 2 倍放大保存为 48×48，用**九宫格（border-image slice 16）**拉伸，
所以能任意尺寸使用而不失真 —— 这正是 GBA 游戏画窗口的方式。

## 5. 像素字体 `fonts/`

来源：**缝合像素字体 Fusion Pixel Font**（12px 比例版，`zh_hans`）
https://github.com/TakWolf/fusion-pixel-font · SIL OFL 1.1（见同目录 `OFL.txt`）

- 原始 TTF 6.7 MB，按界面实际用到的 545 个字符（含 425 个汉字）**子集化**后：
  - `fusion-pixel-12px-zh_hans.ttf` 83 KB —— Qt 用（`QFontDatabase::addApplicationFont`）
  - `fusion-pixel-12px-zh_hans.woff2` 19 KB —— 网页预览内嵌用
- 使用要点：字号取 **12px 的整数倍**（12 / 24 / 36），否则像素栅格会被破坏而发虚。

## 6. TD 图块与技能特效 `tiles/summer|winter` `fx/` `icons/`

来源：**`ElasticSea/PokemonTD`**（LibGDX 实现的宝可梦塔防，原型是魔兽 III 自定义地图
elementalTD），仓库 `unprocessed/` 目录下的原始贴图。由 `tools/import_pokemontd_assets.py`
导入并加工：

| 目录 | 内容 |
|---|---|
| `tiles/summer/` `tiles/winter/` | 48×48 TD 地表：草地、**16 种按四邻掩码预合成的道路图块** `road_0..15`（位序 N=1 E=2 S=4 W=8）、树、入口 |
| `fx/` | 投射物与命中贴图（fire/ice/leaf/waterdrop/bubble/bullet），`fx/seq/` 下是序列帧（火焰 10 帧、火球 5 帧、能量球 13 帧、蓄能 5 帧、藤蔓 2 帧），`fx/seq/electric_*` 是把绿/蓝序列帧**调黄**后的电属性版本 |
| `icons/ability/` | 27 个 64×64 技能图标（魔兽风格浮雕边框） |
| `icons/status/` `icons/gem/` `icons/ui/` | 状态 / 宝石 / 界面图标 |

**加工要点**（都在 `tools/import_pokemontd_assets.py` 里有注释）：

1. 原图块的草地是**青绿色**（蓝 > 红），树冠是**正绿**（蓝 < 红）——必须用这个关系区分，
   否则树冠会被一起改色。
2. 青绿草色全局固定 4 种，按**全图集统一排名**映射到工程的绿宝石草地绿；
   按单块内部排名映射会让不同图块明度基准不一致，拼起来出现深浅色块。
3. **道路图块不能直接叠加**：原图块自带"抖动边缘 + 深色描边"，两块叠加时描边会留在
   路中间，看起来像一圈圈的环。所以离线**预合成 16 种邻接掩码**的道路图块，
   铺路时按"四邻是否也是路"直接选图块，C++/JS/Python 三侧同一套位序。

## 7. 引用规范

用于公开发布、参赛或商业用途时，请将上述素材替换为原创美术资源，核心架构与代码无需改动。
宝可梦相关名称与形象版权归 Nintendo / Creatures Inc. / GAME FREAK inc. 所有。
