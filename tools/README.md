# tools —— Qt 桌面版资源工具

仅维护桌面版，不再生成 HTML 游戏预览。

| 脚本 | 作用 |
|---|---|
| `analyze_forest_map.py` | 生成 `core/ForestMapData.h` 道路中心线、13 个部署点及静态坐标检查图 |
| `build_ui_icons.py` | 生成桌面菜单、按钮的像素图标（SVG 与 PNG） |
| `build_status_icons.py` | 生成元素附着与异常状态像素图标，不含中毒 |
| `fetch_pokeemerald_assets.py` | 下载地表素材原始数据 |
| `import_pokemontd_assets.py` | 导入宝可梦塔防素材 |
| `scan_tile_gradient.py` | 检查平铺图块亮度差 |
| `make_terrain_tiles.py` | 组装地表素材 |
| `make_map_preview.py` | 生成静态地表检查图，不包含游戏逻辑 |

地图数据更新后使用 CMake 重新构建桌面游戏。界面检查使用 `tests/render_layout.cpp`，战斗规则检查使用 `tests/map_rules.cpp`，完整存读档检查使用 `tests/persistence_rules.cpp`。
