#include "AssetManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QStringList>

AssetManager& AssetManager::instance() {
    static AssetManager mgr;
    return mgr;
}

AssetManager::AssetManager() : fontTried(false) {
    baseDir = resolveBaseDir();
}

QString AssetManager::resolveBaseDir() const {
    const QString app = QCoreApplication::applicationDirPath();
    QStringList cands;
    cands << app + "/assets"
          << app + "/../assets"
          << app + "/../../assets"
          << app + "/../../../assets"
          << app + "/../../../../assets"
          << QDir::currentPath() + "/assets"
          << QDir::currentPath() + "/PokemonTowerDefense/assets";

    for (const QString& c : cands) {
        QDir d(c);
        if (d.exists("tiles/summer/grass.png") && d.exists("sprites/pokemon/pichu.png")) {
            return QDir::cleanPath(c);
        }
    }
    return QString();
}

QPixmap AssetManager::load(const QString& relPath) {
    auto it = cache.find(relPath);
    if (it != cache.end()) return it.value();

    QPixmap pm;
    if (!baseDir.isEmpty()) {
        pm.load(baseDir + "/" + relPath);
    }
    cache.insert(relPath, pm);
    return pm;
}

QPixmap AssetManager::pokemon(const QString& className) {
    static const QHash<QString, QString> map = {
        { "Pichu",    "sprites/pokemon/pichu.png" },
        { "Charmander",  "sprites/pokemon/charmander.png" },
        { "Squirtle",   "sprites/pokemon/squirtle.png" },
        { "Bulbasaur",  "sprites/pokemon/bulbasaur.png" },
        { "Pikachu",    "sprites/pokemon/pikachu.png" },
        { "Raichu",     "sprites/pokemon/raichu.png" },
        { "Charmeleon", "sprites/pokemon/charmeleon.png" },
        { "Charizard",  "sprites/pokemon/charizard.png" },
        { "Wartortle",  "sprites/pokemon/wartortle.png" },
        { "Blastoise",  "sprites/pokemon/blastoise.png" },
        { "Ivysaur",    "sprites/pokemon/ivysaur.png" },
        { "Venusaur",   "sprites/pokemon/venusaur.png" },
        { "BasicEnemy", "sprites/pokemon/enemy_basic.png" },
        { "FastEnemy",  "sprites/pokemon/enemy_fast.png" },
        { "TankEnemy",  "sprites/pokemon/enemy_tank.png" },
    };
    auto it = map.find(className);
    if (it == map.end()) return QPixmap();
    return load(it.value());
}

QPixmap AssetManager::tile(const QString& name) {
    return load("tiles/" + name + ".png");
}

QPixmap AssetManager::typeIcon(const QString& typeName) {
    return load("sprites/types/" + typeName.toLower() + ".png");
}

QPixmap AssetManager::fx(const QString& name) {
    return load("fx/" + name + ".png");
}

QPixmap AssetManager::actionIcon(const QString& name) {
    return load("icons/actions/" + name + ".png");
}

QPixmap AssetManager::statusIcon(const QString& name) {
    return load("icons/status_game/" + name.toLower() + ".png");
}

QPixmap AssetManager::mapImage() {
    return load("maps/forest_route_clean.png");
}

QString AssetManager::uiPath(const QString& name) const {
    if (baseDir.isEmpty()) return QString();
    return baseDir + "/ui/" + name;
}

QString AssetManager::fontFamily() {
    if (fontTried) return fontFam;
    fontTried = true;

    QStringList cands;
    if (!baseDir.isEmpty()) cands << baseDir + "/fonts/fusion-pixel-12px-zh_hans.ttf";
    for (const QString& f : cands) {
        if (!QFileInfo::exists(f)) continue;
        int id = QFontDatabase::addApplicationFont(f);
        if (id < 0) continue;
        QStringList fams = QFontDatabase::applicationFontFamilies(id);
        if (!fams.isEmpty()) {
            fontFam = fams.first();
            break;
        }
    }
    return fontFam;
}
