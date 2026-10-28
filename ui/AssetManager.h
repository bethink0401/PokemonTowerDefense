#pragma once
// =============================================================
//  AssetManager.h
//  原生素材加载与缓存（宝可梦立绘 / 绿宝石地面图块 / 属性图标）
//  找不到素材时返回空 QPixmap，调用方退化到程序化绘制
// =============================================================
#include <QString>
#include <QPixmap>
#include <QHash>

class AssetManager {
public:
    static AssetManager& instance();

    // 宝可梦立绘：按类名取（Pichu / Charmander / Squirtle / Bulbasaur /
    //                          BasicEnemy / FastEnemy / TankEnemy）
    QPixmap pokemon(const QString& className);

    // 地面图块：grass / sand / water / tree / flower
    QPixmap tile(const QString& name);

    // 属性图标：Fire / Water / Grass / Electric
    QPixmap typeIcon(const QString& typeName);

    // 攻击特效贴图（assets/fx/ 下）：fire / ice / leaf / waterdrop / bubble ...
    QPixmap fx(const QString& name);
    QPixmap actionIcon(const QString& name);
    QPixmap statusIcon(const QString& name);

    // 战场地图整图（assets/maps/forest_route.png）
    QPixmap mapImage();

    // UI 九宫格边框素材路径：ui/window_white.png 等
    QString uiPath(const QString& name) const;

    // 像素字体族名（首次调用时加载 assets/fonts 下的 TTF；失败返回空串）
    QString fontFamily();

    bool available() const { return !baseDir.isEmpty(); }
    QString dir() const { return baseDir; }

private:
    AssetManager();
    QString resolveBaseDir() const;
    QPixmap load(const QString& relPath);

    QString baseDir;
    QHash<QString, QPixmap> cache;
    QString fontFam;
    bool fontTried;
};
