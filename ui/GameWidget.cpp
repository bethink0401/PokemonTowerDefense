#include "GameWidget.h"
#include "AssetManager.h"
#include "ForestMapData.h"
#include "Game.h"
#include "Pokemon.h"
#include "Enemy.h"
#include "Core.h"
#include "GameMap.h"
#include "EventManager.h"
#include "GameEvent.h"
#include "WaveManager.h"
#include "Statistics.h"
#include "TargetStrategy.h"
#include "ElementStatus.h"
#include "StatusEffect.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPolygonF>
#include <QLineF>
#include <QFontMetrics>
#include <QStringList>
#include <QtMath>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <algorithm>

static const int GRID_W = Position::MAP_WIDTH;
static const int GRID_H = Position::MAP_HEIGHT;
static const int BORDER = 1;                      // 外圈树林

// 确定性哈希：与 Python/JS 侧保持一致，用来选地表变体（同一格子永远同一种草）
static inline quint32 cellHash(int x, int y, int salt) {
    quint32 v = static_cast<quint32>(x * 0x9E3779B1u)
              ^ static_cast<quint32>(y * 0x85EBCA77u)
              ^ static_cast<quint32>(salt * 0xC2B2AE3Du);
    v ^= v >> 15; v *= 0x2545F491u;
    v ^= v >> 13; v *= 0x27D4EB2Fu;
    v ^= v >> 16;
    return v;
}

// 四种草地按权重分布：基准草地出现最多
static const char* GRASS_NAMES[4] = { "grass1", "grass2", "grass3", "grass4" };

// 点到线段距离（用于把路径换算成"土路格子"）
static double distToSegment(double px, double py, double x1, double y1, double x2, double y2) {
    double dx = x2 - x1, dy = y2 - y1;
    double L2 = dx * dx + dy * dy;
    double t = 0.0;
    if (L2 > 1e-9) {
        t = ((px - x1) * dx + (py - y1) * dy) / L2;
        t = std::max(0.0, std::min(1.0, t));
    }
    double cx = x1 + t * dx, cy = y1 + t * dy;
    return std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
}

GameWidget::GameWidget(QWidget* parent)
    : QWidget(parent),
      game(nullptr),
      selected(nullptr),
      roadReady(false),
      terrainCellPx(0),
      hovering(false),
      hoverCell(0, 0),
      timeAcc(0.0),
      shakeTimer(0.0),
      shakeMagnitude(0.0),
      subscriptionGeneration(0) {
    setMinimumSize(720, 320);
    setMouseTracking(true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
}

GameWidget::~GameWidget() {}

// ===================== 素材缓存 =====================
QPixmap GameWidget::tileScaled(const QString& name, double cell) {
    int px = std::max(4, static_cast<int>(std::lround(cell)));
    QString key = name + '#' + QString::number(px);
    auto it = tileCache.find(key);
    if (it != tileCache.end()) return it.value();

    QPixmap src = AssetManager::instance().tile(name);
    QPixmap out;
    if (!src.isNull()) {
        out = src.scaled(px + 1, px + 1, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    tileCache.insert(key, out);
    return out;
}

QRect GameWidget::spriteContentRect(const QString& className) {
    auto it = spriteRectCache.find(className);
    if (it != spriteRectCache.end()) return it.value();

    QRect r;
    QPixmap src = AssetManager::instance().pokemon(className);
    if (!src.isNull()) {
        QImage im = src.toImage().convertToFormat(QImage::Format_ARGB32);
        int x0 = im.width(), y0 = im.height(), x1 = -1, y1 = -1;
        for (int y = 0; y < im.height(); ++y) {
            const QRgb* line = reinterpret_cast<const QRgb*>(im.constScanLine(y));
            for (int x = 0; x < im.width(); ++x) {
                if (qAlpha(line[x]) > 8) {
                    if (x < x0) x0 = x;
                    if (x > x1) x1 = x;
                    if (y < y0) y0 = y;
                    if (y > y1) y1 = y;
                }
            }
        }
        if (x1 >= 0) r = QRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    }
    spriteRectCache.insert(className, r);
    return r;
}

QPixmap GameWidget::spriteScaled(const QString& className, int heightPx, bool white) {
    QString key = className + '#' + QString::number(heightPx) + (white ? "#w" : "#n");
    auto it = spriteCache.find(key);
    if (it != spriteCache.end()) return it.value();

    QPixmap src = AssetManager::instance().pokemon(className);
    QPixmap out;
    if (!src.isNull()) {
        int w = std::max(1, static_cast<int>(src.width() * (heightPx / static_cast<double>(src.height()))));
        out = src.scaled(w, heightPx, Qt::KeepAspectRatio, Qt::FastTransformation);   // 像素图：最近邻缩放
        if (white) {
            QPainter tp(&out);
            tp.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            tp.fillRect(out.rect(), QColor(255, 255, 255, 150));
            tp.end();
        }
    }
    spriteCache.insert(key, out);
    return out;
}

QFont GameWidget::uiFont(int px, bool bold) const {
    QFont f = font();
    QString fam = "Microsoft YaHei";
    if (!fam.isEmpty()) f.setFamily(fam);
    f.setPixelSize(std::max(14, px));
    f.setBold(bold);
    return f;
}

void GameWidget::drawHpBar(QPainter& p, double x, double y, double w, double ratio,
                           const QColor& color) {
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    const double h = 6.0;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x20, 0x20, 0x2a));
    p.drawRect(QRectF(x - 1, y - 1, w + 2, h + 2));
    p.setBrush(QColor(0x5a, 0x5a, 0x68));
    p.drawRect(QRectF(x, y, w, h));
    QColor c = color.isValid() ? color
              : (ratio > 0.5 ? QColor(0x78, 0xC8, 0x50)
                 : (ratio > 0.2 ? QColor(0xF8, 0xD0, 0x30) : QColor(0xF8, 0x58, 0x88)));
    double fw = w * ratio;
    p.setBrush(c);
    p.drawRect(QRectF(x, y, fw, h));
    p.setBrush(QColor(255, 255, 255, 128));
    p.drawRect(QRectF(x, y, fw, 2.0));
}

// ===================== 坐标换算 =====================
double GameWidget::cellSize() const {
    return std::min(width() / double(GRID_W), height() / double(GRID_H));
}

QRectF GameWidget::canvasRect() const {
    return QRectF(rect());
}

QPointF GameWidget::gridToPixel(double gx, double gy) const {
    const QRectF r = canvasRect();
    return QPointF(r.left() + (gx + 0.5) * r.width() / GRID_W,
                   r.top() + (gy + 0.5) * r.height() / GRID_H);
}

QPoint GameWidget::gridFromPixel(const QPointF& p) const {
    const QRectF r = canvasRect();
    int gx = int(std::floor((p.x() - r.left()) * GRID_W / r.width()));
    int gy = int(std::floor((p.y() - r.top()) * GRID_H / r.height()));
    return QPoint(std::clamp(gx, 0, GRID_W - 1), std::clamp(gy, 0, GRID_H - 1));
}

QColor GameWidget::typeColor(int typeIndex) const {
    switch (typeIndex) {
    case 0: return QColor(0xE8, 0x5D, 0x2A);   // Fire
    case 1: return QColor(0x2F, 0x8F, 0xE0);   // Water
    case 2: return QColor(0x3A, 0xA8, 0x4E);   // Grass
    case 3: return QColor(0xE0, 0xB4, 0x1E);   // Electric
    }
    return QColor(0xCC, 0xCC, 0xCC);
}


void GameWidget::computeRoadCells() {
    roadCells.clear();
    decoCells.clear();
    roadFlag.assign(GRID_W * GRID_H, 0);
    if (game) {
        const std::vector<std::vector<Position>>& paths = game->getMap().getPaths();
        for (int y = 0; y < GRID_H; ++y) {
            for (int x = 0; x < GRID_W; ++x) {
                double best = 1e9;
                for (const std::vector<Position>& path : paths) {
                    for (size_t i = 0; i + 1 < path.size(); ++i) {
                        best = std::min(best, distToSegment(x, y,
                                     path[i].getX(), path[i].getY(),
                                     path[i + 1].getX(), path[i + 1].getY()));
                    }
                }
                if (best < 0.60) {
                    roadCells.push_back(QPoint(x, y));
                    roadFlag[y * GRID_W + x] = 1;
                }
            }
        }
    }
    // 草地上的点缀（花丛 / 灌木），避开路面
    for (int y = 0; y < GRID_H; ++y) {
        for (int x = 0; x < GRID_W; ++x) {
            if (roadFlag[y * GRID_W + x]) continue;
            quint32 h = cellHash(x, y, 3) % 31;
            if (h == 0) decoCells.push_back(QPoint(x, y));
            else if (h == 3) decoCells.push_back(QPoint(x, y));
        }
    }
    roadReady = true;
}

bool GameWidget::isRoadCell(int x, int y) const {
    if (x < 0 || y < 0 || x >= GRID_W || y >= GRID_H) return false;
    if (roadFlag.empty()) return false;
    return roadFlag[y * GRID_W + x] != 0;
}

// 把整张地表（草变体 + 道路 + 树木 + 点缀）预渲染成一张图，之后每帧只贴一次
void GameWidget::rebuildTerrain() {
    double cell = cellSize();
    int px = std::max(4, static_cast<int>(std::lround(cell)));
    int w = px * (GRID_W + 2 * BORDER);
    int h = px * (GRID_H + 2 * BORDER);
    if (w <= 0 || h <= 0) return;

    terrainCache = QPixmap(w, h);
    terrainCache.fill(Qt::transparent);
    QPainter t(&terrainCache);
    t.setRenderHint(QPainter::SmoothPixmapTransform, false);

    auto tile = [&](const QString& n) { return tileScaled(n, px); };
    QPixmap grass = tile("summer/grass");
    QPixmap flower = tile("flower");
    QPixmap bush = tile("bush");
    QPixmap trA = tile("summer/tree");
    QPixmap trB = tile("summer/tree_sub");
    QPixmap entrance = tile("summer/entrance");

    // 16 种道路图块（离线预合成，位序 N=1 E=2 S=4 W=8，与预览页/工具脚本一致）
    QPixmap roads[16];
    for (int m = 0; m < 16; ++m) roads[m] = tile("summer/road_" + QString::number(m));

    auto cellRect = [&](int cx, int cy) {
        return QRectF((cx + BORDER) * px, (cy + BORDER) * px, px, px);
    };
    auto put = [&](const QPixmap& pm, int cx, int cy) {
        if (!pm.isNull()) t.drawPixmap(cellRect(cx, cy), pm, pm.rect());
    };

    if (!roadReady) computeRoadCells();

    // 场地草地 + 道路
    for (int y = 0; y < GRID_H; ++y) {
        for (int x = 0; x < GRID_W; ++x) {
            put(grass, x, y);
            if (!isRoadCell(x, y)) continue;
            int m = 0;
            if (isRoadCell(x, y - 1)) m |= 1;
            if (isRoadCell(x + 1, y)) m |= 2;
            if (isRoadCell(x, y + 1)) m |= 4;
            if (isRoadCell(x - 1, y)) m |= 8;
            put(roads[m], x, y);
        }
    }

    // 点缀
    for (const QPoint& c : decoCells) {
        quint32 hh = cellHash(c.x(), c.y(), 3) % 31;
        put((hh == 0) ? flower : bush, c.x(), c.y());
    }

    // 外圈：先铺草再叠树
    for (int cy = -BORDER; cy < GRID_H + BORDER; ++cy) {
        for (int cx = -BORDER; cx < GRID_W + BORDER; ++cx) {
            if (cx >= 0 && cx < GRID_W && cy >= 0 && cy < GRID_H) continue;
            put(grass, cx, cy);
            put((cellHash(cx, cy, 2) % 2) ? trB : trA, cx, cy);
        }
    }

    // 出生点入口
    if (!entrance.isNull() && !game->getMap().getPaths().empty()) {
        Position sp = game->getMap().getPaths().front().front();
        put(entrance, sp.getX(), sp.getY());
    }

    t.end();
    terrainCellPx = px;
}

void GameWidget::resetBattleVisuals() {
    particles.clear();texts.clear();flashes.clear();reactionFx.clear();lastEnemyHp.clear();
    shakeTimer=0;shakeMagnitude=0;lastEvolveSeen=game?game->getEvolutionCount():0;
    update();
}

void GameWidget::setGame(Game* g) {
    if(game==g) return;
    game = g;
    const unsigned generation=++subscriptionGeneration;
    roadReady = false;
    terrainCache = QPixmap();      // 地图变了，地表缓存作废
    terrainCellPx = 0;
    if (game) {
        game->getEventManager().subscribe([this,generation](GameEvent& e) {
            if(generation!=subscriptionGeneration) return;
            switch (e.getType()) {
            case EventType::EnemyDead: {
                auto& ev = static_cast<EnemyDeadEvent&>(e);
                spawnBurst(ev.getPosition().getX(), ev.getPosition().getY(),
                           QColor(0xFF, 0xC8, 0x5A), 14, 5.0);
                addText(ev.getPosition().getX(), ev.getPosition().getY(),
                        "+" + QString::number(ev.getReward()), QColor(0xFF, 0xD7, 0x6E));
                break;
            }
            case EventType::ReactionTriggered: {
                auto& ev = static_cast<ReactionTriggeredEvent&>(e);
                // 每种反应一种专属颜色（与预览页 REACTION_COLOR 一致）
                static const std::unordered_map<std::string, int> kReactionColor = {
                    {"电导链", 0xFFF06A},
                    {"燃烧", 0xFF8A3D}, {"缠绕生长", 0x8FE07A}, {"能量爆破", 0xFFD84A},
                    {"蒸汽爆发", 0xE8F2F8}, {"麻痹孢子", 0xCFFF5A},
                };
                auto it = kReactionColor.find(ev.getReactionName());
                QColor col = (it != kReactionColor.end())
                                 ? QColor(it->second) : QColor(0xFF, 0xFF, 0xFF);
                ReactionFx fx;
                fx.name=QString::fromStdString(ev.getReactionName());
                fx.x = ev.getPosition().getX();
                fx.y = ev.getPosition().getY();
                fx.maxLife = 0.4;
                fx.life = fx.maxLife;
                fx.color = col;
                reactionFx.push_back(fx);
                spawnBurst(ev.getPosition().getX(), ev.getPosition().getY(), col, 26, 7.5);
                addText(ev.getPosition().getX(), ev.getPosition().getY() - 1.0,
                        QString::fromStdString(ev.getReactionName()), col, true, 0.3);
                break;
            }
            case EventType::CoreDamaged: {
                shakeTimer = 0.25;
                shakeMagnitude = 7.0;
                break;
            }
            case EventType::WaveClear: {
                addText(game->getMap().getCorePosition().getX(),
                        game->getMap().getCorePosition().getY() - 1.0,
                        "波次清空", QColor(0x7C, 0xFF, 0xB0), true);
                break;
            }
            }
        });
    }
}

void GameWidget::setDeployType(const std::string& type) {
    deployType = type;
    update();
}

void GameWidget::clearDeployType() {
    deployType.clear();
    update();
}

void GameWidget::setSelected(Pokemon* p) {
    selected = p;
    if (onSelectionChanged) onSelectionChanged(p);
    update();
}

void GameWidget::spawnBurst(double gx, double gy, const QColor& color, int count, double speed) {
    for (int i = 0; i < count; ++i) {
        double ang = (2.0 * 3.14159265358979 * i) / count + (std::rand() % 100) / 100.0;
        double sp = speed * (0.5 + (std::rand() % 100) / 100.0);
        Particle pt;
        pt.x = gx; pt.y = gy;
        pt.vx = std::cos(ang) * sp;
        pt.vy = std::sin(ang) * sp;
        pt.maxLife = 0.4 + (std::rand() % 40) / 100.0;
        pt.life = pt.maxLife;
        pt.color = color;
        pt.size = 2.0 + (std::rand() % 30) / 10.0;
        particles.push_back(pt);
    }
}

void GameWidget::addText(double gx, double gy, const QString& text, const QColor& color, bool big, double duration) {
    FloatingText t;
    t.x = gx; t.y = gy;
    t.text = text;
    t.life = duration>0 ? duration : (big ? 1.55 : 0.85);
    t.color = color;
    t.big = big;
    texts.push_back(t);
}

void GameWidget::detectCombatEffects() {
    if (!game) return;
    auto& enemies = game->getMap().getEnemies();

    std::unordered_map<const Enemy*, int> current;
    for (Enemy* e : enemies) {
        if (!e) continue;
        current[e] = e->getHp();
        auto it = lastEnemyHp.find(e);
        if (it != lastEnemyHp.end() && e->getHp() < it->second) {
            int dmg = it->second - e->getHp();
            QColor c = typeColor(static_cast<int>(e->getType()));
            spawnBurst(e->getX(), e->getY(), c, 6, 3.5);
            addText(e->getX(), e->getY() - 0.3, QString::number(dmg),
                    (dmg >= 30 ? QColor(0xFF, 0xE0, 0x66) : QColor(0xFF, 0xFF, 0xFF)));
        }
    }
    lastEnemyHp.swap(current);

    // 攻击特效：按元素区分形态
    for (Pokemon* p : game->getMap().getDefenders()) {
        if (!p || !p->didAttack()) continue;
        AttackFlash f;
        f.x1 = p->getPosition().getX();
        f.y1 = p->getPosition().getY() - 0.8;
        f.x2 = p->getLastTargetPos().getX();
        f.y2 = p->getLastTargetPos().getY() - 0.6;
        f.element = QString::fromStdString(typeToString(p->getType()));
        f.color = typeColor(static_cast<int>(p->getType()));
        f.maxLife = f.element == "Fire" ? 0.55 : 0.45;
        f.life = f.maxLife;
        flashes.push_back(f);
        // 命中点爆散
        spawnBurst(f.x2, f.y2, f.color, 7, 4.5);
    }
}

void GameWidget::updateEffects(double dt) {
    for (size_t i = 0; i < particles.size();) {
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
        particles[i].vy += 6.0 * dt;
        particles[i].life -= dt;
        if (particles[i].life <= 0) particles.erase(particles.begin() + static_cast<long>(i));
        else ++i;
    }
    for (size_t i = 0; i < texts.size();) {
        texts[i].y -= dt * 0.7;
        texts[i].life -= dt;
        if (texts[i].life <= 0) texts.erase(texts.begin() + static_cast<long>(i));
        else ++i;
    }
    for (size_t i = 0; i < flashes.size();) {
        flashes[i].life -= dt;
        if (flashes[i].life <= 0) flashes.erase(flashes.begin() + static_cast<long>(i));
        else ++i;
    }
    for (size_t i = 0; i < reactionFx.size();) {
        reactionFx[i].life -= dt;
        if (reactionFx[i].life <= 0) reactionFx.erase(reactionFx.begin() + static_cast<long>(i));
        else ++i;
    }
    if (shakeTimer > 0) shakeTimer -= dt;
}

void GameWidget::tick(double dt) {
    timeAcc += dt;
    if (game) {
        game->update(dt);
        detectCombatEffects();
        // 进化特效：轮询 Game 的进化计数，变化即在原地播放粒子 + 飘字
        if (game->getEvolutionCount() != lastEvolveSeen) {
            lastEvolveSeen = game->getEvolutionCount();
            const Position& pos = game->getLastEvolvePosition();
            spawnBurst(pos.getX(), pos.getY(), QColor(0xFF, 0xE2, 0x4A), 28, 5.0);
            addText(pos.getX(), pos.getY() - 0.4,
                    QString::fromUtf8("进化！"), QColor(0xFF, 0xE2, 0x4A), true);
        }
    }
    updateEffects(dt);
    update();
}

// ===================== 绘制 =====================

void GameWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    p.save();
    if (shakeTimer > 0) {
        double k = shakeTimer / 0.25;
        double ox = (std::rand() % 200 - 100) / 100.0 * shakeMagnitude * k;
        double oy = (std::rand() % 200 - 100) / 100.0 * shakeMagnitude * k;
        p.translate(ox, oy);
    }

    drawBackground(p);
    drawPaths(p);
    drawCore(p);
    drawPokemon(p);
    drawEnemies(p);
    drawEffects(p);
    drawDeployPreview(p);
    drawOverlay(p);
    p.restore();
}

void GameWidget::drawBackground(QPainter& p) {
    // 浅色底：未铺到地表的地方也不会露出深色
    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0.0, QColor(0xDC, 0xE8, 0xF6));
    bg.setColorAt(1.0, QColor(0xC6, 0xD8, 0xEE));
    p.fillRect(rect(), bg);

    double cell = cellSize();
    QRectF field = canvasRect();

    // 森林路线图：整图直接铺在 46x23 的战场格上
    QPixmap mapImg = AssetManager::instance().mapImage();
    if (!mapImg.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(field, mapImg, QRectF(mapImg.rect()));
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    } else {
        // 无地图素材时退化为旧的程序化地表
        int px = std::max(4, static_cast<int>(std::lround(cell)));
        if (terrainCache.isNull() || terrainCellPx != px) rebuildTerrain();
        QRectF outer(field.left() - cell * BORDER, field.top() - cell * BORDER,
                     field.width() + cell * 2 * BORDER, field.height() + cell * 2 * BORDER);
        if (!terrainCache.isNull()) {
            p.setRenderHint(QPainter::SmoothPixmapTransform, false);
            p.drawPixmap(outer, terrainCache, QRectF(terrainCache.rect()));
        }
    }

    if (game && !deployType.empty()) {
        for (int y = 0; y < GRID_H; ++y)
            for (int x = 0; x < GRID_W; ++x)
                if (game->getMap().isTallGrass(Position(x,y)) && !game->isPositionOccupied(Position(x,y))) {
                    QPointF c = gridToPixel(x,y);
                    p.setBrush(QColor(19,87,62,100));
                    p.setPen(QPen(QColor(240,255,230,230),2));
                    p.drawEllipse(c,cell*.43,cell*.26);
                    p.setPen(QPen(QColor(255,255,230,230),2));
                    p.drawLine(c+QPointF(-4,0),c+QPointF(4,0));
                    p.drawLine(c+QPointF(0,-4),c+QPointF(0,4));
                }
    }

    // 场地描边
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0, 0, 0, 80), 2));
    p.drawRect(field);

    if (!AssetManager::instance().available()) {
        p.setPen(QColor(0xB0, 0x30, 0x30));
        p.setFont(uiFont(12, false));
        p.drawText(QRectF(12, 10, 420, 18), Qt::AlignLeft,
                   "未找到 assets 素材目录，已退化为程序化绘制");
    }
}

void GameWidget::drawPaths(QPainter& p) {
    if (!game) return;
    double cell = cellSize();
    double pulse = 0.6 + 0.4 * std::sin(timeAcc * 3.0);

    // 两个刷怪点传送门：左上山洞、左下路尽头（与 ForestMapData 一致）
    const FWaypoint c0 = ForestMap::pathCave().front();
    const FWaypoint d0 = ForestMap::pathDen().front();
    const QPointF portals[] = { gridToPixel(c0.x, c0.y), gridToPixel(d0.x, d0.y) };
    for (const QPointF& sp : portals) {
        QRadialGradient g(sp, cell * 0.8 * pulse);
        g.setColorAt(0.0, QColor(0x40, 0x20, 0x10, 230));
        g.setColorAt(0.5, QColor(0x30, 0x18, 0x0C, 120));
        g.setColorAt(1.0, QColor(0x20, 0x10, 0x08, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawEllipse(sp, cell * 0.55 * pulse, cell * 0.55 * pulse);
        p.setPen(QPen(QColor(0x18, 0x0C, 0x06, 230), 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(sp, cell * 0.34, cell * 0.34);
    }
}

void GameWidget::drawCore(QPainter& p) {
    if (!game) return;
    Core& core = game->getMap().getCore();
    const FWaypoint& dp = game->getMap().getDoorPoint();
    QPointF c = gridToPixel(dp.x, dp.y);
    double cell = cellSize();
    double pulse = 0.85 + 0.15 * std::sin(timeAcc * 2.5);
    bool damaged = core.getHitFlashTimer() > 0;

    QColor coreColor = damaged ? QColor(0xFF, 0x55, 0x55) : QColor(0x63, 0xE8, 0xFF);

    QRadialGradient glow(c, cell * 1.2 * pulse);
    glow.setColorAt(0.0, QColor(coreColor.red(), coreColor.green(), coreColor.blue(), 170));
    glow.setColorAt(0.5, QColor(coreColor.red(), coreColor.green(), coreColor.blue(), 55));
    glow.setColorAt(1.0, QColor(coreColor.red(), coreColor.green(), coreColor.blue(), 0));
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(c, cell * 1.2 * pulse, cell * 1.2 * pulse);

    QPolygonF hex;
    for (int i = 0; i < 6; ++i) {
        double ang = 3.14159265 / 3.0 * i - 3.14159265 / 6.0;
        hex << QPointF(c.x() + std::cos(ang) * cell * 0.42,
                       c.y() + std::sin(ang) * cell * 0.42);
    }
    QLinearGradient cg(c.x() - cell, c.y() - cell, c.x() + cell, c.y() + cell);
    cg.setColorAt(0.0, coreColor.lighter(140));
    cg.setColorAt(1.0, coreColor.darker(140));
    p.setBrush(cg);
    p.setPen(QPen(QColor(255, 255, 255, 210), 2));
    p.drawPolygon(hex);

    double bw = cell * 1.6;
    QColor barColor = damaged ? QColor(0xFF, 0x5A, 0x5A)
                              : (core.getHpRatio() > 0.5 ? QColor(0x63, 0xE8, 0xFF)
                                 : (core.getHpRatio() > 0.25 ? QColor(0xFF, 0xD7, 0x4A)
                                                             : QColor(0xFF, 0x5A, 0x5A)));
    double barY = c.y() - cell * 0.86;
    drawHpBar(p, c.x() - bw / 2, barY, bw, core.getHpRatio(), barColor);

    // 深色底衬保证在草地上可读
    QRectF tag(c.x() - 46, barY - 18, 92, 16);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x1b, 0x1b, 0x24, 215));
    p.drawRect(tag);
    p.setPen(QColor(0xE6, 0xF4, 0xFF));
    p.setFont(uiFont(12, true));
    p.drawText(tag, Qt::AlignCenter, QString("CORE %1").arg(core.getHp()));
}

bool GameWidget::drawUnitSprite(QPainter& p, const QString& className, const QPointF& ground,
                                double targetH, bool flash, bool friendly) {
    QRect cr = spriteContentRect(className);
    QPixmap src = AssetManager::instance().pokemon(className);
    if (src.isNull() || cr.isNull()) return false;

    const double scale = targetH / cr.height();          // 让"可见内容"高度 = targetH
    const int dh = std::max(1, static_cast<int>(std::lround(src.height() * scale)));
    QPixmap pm = spriteScaled(className, dh, flash);     // 整图等比缩放（走缓存）
    if (pm.isNull()) return false;

    const double left = ground.x() - (cr.x() + cr.width() / 2.0) * scale;
    const double top  = ground.y() - (cr.y() + cr.height()) * scale;
    if (friendly) {
        auto silhouette = [&](const QString& suffix, const QColor& color) {
            QString key = className + "#" + QString::number(dh) + suffix;
            if (!spriteCache.contains(key)) {
                QPixmap mask = pm;
                QPainter painter(&mask);
                painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                painter.fillRect(mask.rect(),color);
                painter.end();
                spriteCache.insert(key,mask);
            }
            return spriteCache.value(key);
        };
        QPixmap ink = silhouette("#allyInk",QColor(18,65,101,235));
        QPixmap rim = silhouette("#allyRim",QColor(250,255,241,255));
        for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) if(dx||dy)
            p.drawPixmap(QPointF(left+dx*2,top+dy*2),ink);
        for(const QPointF& offset : {QPointF(-1,0),QPointF(1,0),QPointF(0,-1),QPointF(0,1)})
            p.drawPixmap(QPointF(left,top)+offset,rim);
    }
    p.drawPixmap(QPointF(left, top), pm);
    return true;
}

void GameWidget::drawPokemon(QPainter& p) {
    if (!game) return;
    double cell = cellSize();
    for (Pokemon* pk : game->getMap().getDefenders()) {
        if (!pk) continue;
        QPointF c = gridToPixel(pk->getPosition().getX(), pk->getPosition().getY());
        QColor col = typeColor(static_cast<int>(pk->getType()));

        if (pk == selected) {
            p.setBrush(QColor(col.red(), col.green(), col.blue(), 34));
            p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), 170), 2, Qt::DashLine));
            p.drawEllipse(c, pk->getRange() * cell, pk->getRange() * cell);
        }

        // 脚底接地点 = 阴影中心；h 是"可见内容"的高度
        const double groundY = c.y();
        const double h = std::max(32.0, cell * 1.85);

        // The blue trainer base marks allies; selection uses a gold rim.
        p.setPen(QPen(QColor(15,60,95,230),3));
        p.setBrush(QColor(36,139,197,210));
        QRectF base(c.x()-cell*.68,groundY-cell*.22,cell*1.36,cell*.44);
        p.drawEllipse(base);
        p.setPen(QPen(pk==selected ? QColor(255,215,87) : QColor(226,250,255),2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(base.adjusted(2,1,-2,-1));
        if(pk==selected) {
            p.setPen(QPen(QColor(255,215,87),2));
            p.drawLine(QPointF(c.x()-cell*.72,groundY-h*.72),QPointF(c.x()-cell*.72,groundY-h*.38));
            p.drawLine(QPointF(c.x()+cell*.72,groundY-h*.72),QPointF(c.x()+cell*.72,groundY-h*.38));
        }

        QString cls = QString::fromStdString(pk->getClassName());
        if (!drawUnitSprite(p, cls, QPointF(c.x(), groundY), h, false, true)) {
            // 无素材：程序化绘制
            QRadialGradient halo(c, cell * 0.7);
            halo.setColorAt(0.0, QColor(col.red(), col.green(), col.blue(), 90));
            halo.setColorAt(1.0, QColor(col.red(), col.green(), col.blue(), 0));
            p.setPen(Qt::NoPen);
            p.setBrush(halo);
            p.drawEllipse(c, cell * 0.7, cell * 0.7);
            drawCreature(p, c, cell * 0.45, col, false, false);
        }

        // 等级点
        p.setBrush(QColor(0xFF, 0xD7, 0x4A));
        for (int i = 0; i < pk->getLevel(); ++i) {
            p.drawEllipse(QPointF(c.x() - cell * 0.28 + i * cell * 0.14, groundY + 9), 2.0, 2.0);
        }

        // A compact element symbol, rather than a tiny text-bearing badge.
        QPixmap icon = AssetManager::instance().statusIcon(QString::fromStdString(typeToString(pk->getType())));
        if (!icon.isNull()) {
            const double size=16;
            p.drawPixmap(QRectF(c.x()-size/2,groundY-h-size-3,size,size),icon,icon.rect());
        }
    }
}

void GameWidget::drawEnemies(QPainter& p) {
    if (!game) return;
    Enemy* target=selected?selected->selectTarget(game->getMap().getEnemies()):nullptr;
    double cell = cellSize();
    for (Enemy* e : game->getMap().getEnemies()) {
        if (!e) continue;
        QPointF c = gridToPixel(e->getX(), e->getY());
        QColor col = typeColor(static_cast<int>(e->getType()));
        bool isTank = (e->getClassName() == "TankEnemy");

        const double groundY = c.y();
        const double h = std::max(25.0, cell * (isTank ? 1.65 : 1.40));
        const double top = groundY - h;

        if(e==target) {
            p.setPen(QPen(QColor(255,224,100),2));p.setBrush(QColor(255,219,68,45));
            p.drawEllipse(QRectF(c.x()-cell*.72,groundY-cell*.3,cell*1.44,cell*.6));
            p.setBrush(QColor(255,224,100));p.setPen(QPen(QColor(91,56,22),1));
            QPolygonF arrow;arrow<<QPointF(c.x()-5,top-20)<<QPointF(c.x()+5,top-20)<<QPointF(c.x(),top-14);
            p.drawPolygon(arrow);
        }

        // 地面阴影
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 82));
        p.drawEllipse(QPointF(c.x(), groundY),
                      cell * (isTank ? 0.50 : 0.40), cell * 0.13);

        QString cls = QString::fromStdString(e->getClassName());
        if (!drawUnitSprite(p, cls, QPointF(c.x(), groundY), h, e->getHitFlashTimer() > 0)) {
            drawCreature(p, c, cell * (isTank ? 0.5 : 0.4), col, true, e->getHitFlashTimer() > 0);
        }

        // 宝可梦风格血条（头顶上方）
        drawHpBar(p, c.x() - cell * 0.48, top - 10, cell * 0.96, e->getHpRatio());

        // One icon per distinct status, capped at three; effects still refresh.
        QStringList badges;
        if (auto* element=e->getPrimaryElement())
            badges << QString::fromStdString(typeToString(element->getType())).toLower();
        for(StatusEffect* effect : e->getStatusEffects()) {
            const auto name=effect->getClassName();
            QString key=name=="SlowEffect" ? "slow" : 
                        name=="BurnEffect" ? "burn" : "paralyze";
            if(!badges.contains(key)) badges << key;
        }
        int count=std::min(3,int(badges.size()));
        for(int i=0;i<count;++i) {
            QRectF box(c.x()-(count*20-2)/2.0+i*20,groundY+4,18,18);
            p.setPen(QPen(QColor(228,244,255,220),1));
            p.setBrush(QColor(20,40,65,215));
            p.drawRoundedRect(box,2,2);
            QPixmap icon=AssetManager::instance().statusIcon(badges[i]);
            if(!icon.isNull()) p.drawPixmap(box.adjusted(1,1,-1,-1),icon,icon.rect());
        }

        // 冻结 / 燃烧 状态光环
        bool frozen = false, burning = false;
        for (StatusEffect* s : e->getStatusEffects()) {
            if (s->isImmobilize()) frozen = true;
            if (s->getClassName() == "BurnEffect") burning = true;
        }
        if (frozen) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(160, 230, 255, 82));
            p.drawEllipse(c, h * 0.45, h * 0.45);
        }
        if (burning) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 120, 40, 78));
            p.drawEllipse(c, h * 0.45, h * 0.45);
        }
    }
}

void GameWidget::drawReactionEffect(QPainter& p, const ReactionFx& effect) {
    const double k=effect.life/effect.maxLife, progress=1-k;
    const double fade=std::min(1.0,k*1.7), cell=cellSize(), pi=3.141592653589793;
    const QPointF c=gridToPixel(effect.x,effect.y-.45);
    auto tint=[&](QColor color,double alpha){ color.setAlphaF(std::clamp(alpha,0.0,1.0));return color; };
    auto point=[&](double angle,double radius){return c+QPointF(std::cos(angle),std::sin(angle))*radius;};
    auto ring=[&](double radius,QColor color,double width){
        p.setBrush(Qt::NoBrush);p.setPen(QPen(tint(color,fade*.8),width));p.drawEllipse(c,radius,radius);
    };
    p.save();
    QRadialGradient glow(c,cell*2.4);
    glow.setColorAt(0,tint(effect.color,fade*.34));glow.setColorAt(1,tint(effect.color,0));
    p.setPen(Qt::NoPen);p.setBrush(glow);p.drawEllipse(c,cell*2.4,cell*2.4);
    ring(cell*(.35+progress*2.2),effect.color,3);
    ring(cell*(.25+progress*1.8),Qt::white,1.3);
    if(effect.name=="能量爆破") {
        QPolygonF star;
        for(int i=0;i<24;++i) star<<point(i*pi/12,cell*(i%2?.45:1.0)*(.6+progress*2));
        p.setBrush(tint(QColor(255,164,57),fade*.38));p.setPen(QPen(tint(QColor(255,241,128),fade),2));p.drawPolygon(star);
        ring(cell*(.5+progress*2.6),QColor(255,120,65),3);
    } else if(effect.name=="电导链") {
        for(int i=0;i<6;++i) {
            QPolygonF bolt;double angle=i*pi/3+progress*.6;
            for(int j=0;j<7;++j) bolt<<point(angle+(j%2?.1:-.1),cell*(.2+j*.23)*(1+progress*.4));
            p.setPen(QPen(tint(QColor(255,235,80),fade),4));p.drawPolyline(bolt);
            p.setPen(QPen(tint(Qt::white,fade),1));p.drawPolyline(bolt);
        }
        int links=0;
        if(game) for(Enemy* other : game->getMap().getEnemies()) {
            if(!other || other->isDead() || links>=3) continue;
            QPointF target=gridToPixel(other->getX(),other->getY()-.6);
            if(QLineF(c,target).length()>cell*4 || QLineF(c,target).length()<cell*.3) continue;
            QPolygonF bolt;QPointF delta=target-c;QPointF perpendicular(-delta.y(),delta.x());double length=std::max(1.0,QLineF(c,target).length());
            for(int j=0;j<=6;++j) bolt<<c+delta*(j/6.0)+perpendicular/length*(j==0||j==6?0:(j%2?1:-1)*cell*.22);
            p.setPen(QPen(tint(QColor(255,242,127),fade),2.5));p.drawPolyline(bolt);++links;
        }
    } else if(effect.name=="燃烧") {
        for(int i=0;i<8;++i) {
            QPointF center=point(i*pi/4,cell*(.25+progress));center.ry()-=cell*progress*.8;
            QPainterPath flame(center+QPointF(0,-cell*.7));
            flame.quadTo(center+QPointF(cell*.55,cell*.3),center+QPointF(0,cell*.35));
            flame.quadTo(center+QPointF(-cell*.45,cell*.25),center+QPointF(0,-cell*.7));
            p.setPen(Qt::NoPen);p.setBrush(tint(QColor(255,126,50),fade*.9));p.drawPath(flame);
            p.setBrush(tint(QColor(255,238,135),fade));p.drawEllipse(center,cell*.13,cell*.25);
        }
    } else if(effect.name=="缠绕生长") {
        for(int i=0;i<6;++i) {
            double angle=i*pi/3+progress*.8;
            QPointF end=point(angle,cell*(.5+progress*1.7));
            QPainterPath vine(c);vine.quadTo(point(angle+.7,cell*1.3),end);
            p.setPen(QPen(tint(QColor(63,175,75),fade),4));p.setBrush(Qt::NoBrush);p.drawPath(vine);
            p.setPen(QPen(tint(QColor(201,255,159),fade),1));p.drawPath(vine);
            p.setPen(Qt::NoPen);p.setBrush(tint(QColor(139,239,102),fade));p.drawEllipse(end,cell*.27,cell*.12);
        }
    } else if(effect.name=="蒸汽爆发") {
        for(int i=0;i<7;++i) {
            QPointF center=point(i*2*pi/7,cell*(.25+progress*1.1));center.ry()-=cell*progress*1.1;
            double radius=cell*(.4+progress*.35);
            QRadialGradient cloud(center,radius);cloud.setColorAt(0,tint(Qt::white,fade*.9));cloud.setColorAt(1,tint(QColor(173,221,245),0));
            p.setPen(Qt::NoPen);p.setBrush(cloud);p.drawEllipse(center,radius,radius);
        }
        ring(cell*(.4+progress*1.9),QColor(255,188,106),2);
    } else if(effect.name=="麻痹孢子") {
        for(int i=0;i<12;++i) {
            QPointF center=point(i*pi/6+progress*.5,cell*(.35+progress*1.7));
            center.ry()+=std::sin(timeAcc*6+i)*cell*.1;
            p.setPen(QPen(tint(QColor(249,255,162),fade),1));p.setBrush(tint(QColor(178,241,93),fade));p.drawEllipse(center,cell*.13,cell*.13);
            if(i%3==0){p.setPen(QPen(tint(QColor(255,245,92),fade),2));p.drawLine(center+QPointF(-3,-5),center+QPointF(2,0));p.drawLine(center+QPointF(2,0),center+QPointF(-1,5));}
        }
    }
    p.restore();
}

void GameWidget::drawEffects(QPainter& p) {
    for (const AttackFlash& f : flashes) drawAttackEffect(p, f);

    for (const ReactionFx& effect : reactionFx) drawReactionEffect(p,effect);

    p.setPen(Qt::NoPen);
    for (const Particle& pt : particles) {
        double k = pt.life / pt.maxLife;
        QColor col = pt.color;
        col.setAlpha(static_cast<int>(220 * k));
        p.setBrush(col);
        QPointF c = gridToPixel(pt.x, pt.y);
        p.drawEllipse(c, pt.size * k, pt.size * k);
    }

    for (const FloatingText& t : texts) {
        double k = std::min(1.0, t.life / 0.5);
        QColor col = t.color;
        col.setAlpha(static_cast<int>(255 * k));
        QFont fnt = uiFont(t.big ? 24 : 12, t.big);
        p.setFont(fnt);
        p.setPen(col);
        QPointF c = gridToPixel(t.x, t.y);
        QFontMetrics fm(fnt);
        double w = fm.horizontalAdvance(t.text);
        QPainterPath letters;
        letters.addText(QPointF(c.x()-w/2,c.y()),fnt,t.text);
        p.strokePath(letters,QPen(QColor(14,29,46,int(235*k)),t.big ? 4 : 2.5,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        p.fillPath(letters,col);
    }
}

// 普通攻击特效：形态与颜色对应元素
void GameWidget::drawAttackEffect(QPainter& p, const AttackFlash& f) {
    double k = f.life / f.maxLife;                 // 1 -> 0
    double cell = cellSize();
    QPointF a = gridToPixel(f.x1, f.y1);
    QPointF b = gridToPixel(f.x2, f.y2);
    QColor col = f.color;
    p.setBrush(Qt::NoBrush);

    p.save();
    QRadialGradient impact(b,cell*1.35);
    impact.setColorAt(0,QColor(255,255,235,int(170*k)));
    impact.setColorAt(.25,QColor(col.red(),col.green(),col.blue(),int(125*k)));
    impact.setColorAt(1,QColor(col.red(),col.green(),col.blue(),0));
    p.setPen(Qt::NoPen);p.setBrush(impact);p.drawEllipse(b,cell*1.35,cell*1.35);
    p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor(255,250,225,int(230*k)),2));
    p.drawEllipse(b,cell*(.2+(1-k)*.65),cell*(.2+(1-k)*.65));
    double travel=std::min(1.0,(1-k)*3.5);
    QPointF shot=a+(b-a)*travel;
    p.setPen(QPen(QColor(255,255,235,int(230*k)),2));
    p.setBrush(QColor(col.red(),col.green(),col.blue(),int(245*k)));
    p.drawEllipse(shot,cell*.19,cell*.19);
    for(int i=0;i<6;++i) {
        double angle=i*3.14159265/3+timeAcc*1.5;
        QPointF direction(std::cos(angle),std::sin(angle));
        p.drawLine(b+direction*cell*(.3+(1-k)*.4),b+direction*cell*(.5+(1-k)*.9));
    }
    p.restore();

    // 命中点叠加原生特效贴图（来源：PokemonTD / elementalTD）
    {
        QString fxName;
        if (f.element == "Electric")    fxName = "bullet";
        else if (f.element == "Fire") fxName = "fire";
        else if (f.element == "Water")  fxName = "waterdrop";
        else if (f.element == "Grass")  fxName = "leaf";
        if (!fxName.isEmpty()) {
            QPixmap sp = AssetManager::instance().fx(fxName);
            if (!sp.isNull()) {
                double s2 = cell * (f.element == "Fire" ? 0.85 : 0.55) * (0.75 + 0.35 * k);
                QRectF r(b.x() - s2 / 2, b.y() - s2 / 2, s2, s2);
                p.setOpacity(std::max(0.0, std::min(1.0, k)));
                p.drawPixmap(r, sp, sp.rect());
                p.setOpacity(1.0);
            }
        }
    }

    if (f.element == "Electric") {
        // 锯齿闪电 + 外发光
        const int N = 7;
        QPolygonF bolt;
        for (int i = 0; i <= N; ++i) {
            double t = i / static_cast<double>(N);
            double x = a.x() + (b.x() - a.x()) * t;
            double y = a.y() + (b.y() - a.y()) * t;
            if (i != 0 && i != N) {
                double off = ((i % 2) ? 1.0 : -1.0) * cell * 0.28;
                double dx = b.x() - a.x(), dy = b.y() - a.y();
                double len = std::sqrt(dx * dx + dy * dy);
                if (len > 1e-6) { x += -dy / len * off; y += dx / len * off; }
            }
            bolt << QPointF(x, y);
        }
        p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), static_cast<int>(90 * k)),
                      cell * 0.34, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(bolt);
        p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), static_cast<int>(240 * k)),
                      cell * 0.10, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(bolt);
        p.setPen(QPen(QColor(255, 255, 255, static_cast<int>(230 * k)),
                      cell * 0.035, Qt::SolidLine, Qt::RoundCap));
        p.drawPolyline(bolt);
    } else if (f.element == "Fire") {
        // 火焰锥：沿方向逐渐扩大
        const int N = 9;
        for (int i = 0; i < N; ++i) {
            double t = i / static_cast<double>(N - 1);
            double x = a.x() + (b.x() - a.x()) * t;
            double y = a.y() + (b.y() - a.y()) * t;
            double rr = cell * (0.14 + t * 0.52) * (0.7 + 0.3 * k);
            QRadialGradient g(QPointF(x, y), rr);
            QColor c1 = (t < 0.45) ? QColor(255, 214, 90) : QColor(255, 120, 40);
            g.setColorAt(0.0, QColor(c1.red(), c1.green(), c1.blue(), static_cast<int>(220 * k)));
            g.setColorAt(1.0, QColor(255, 60, 0, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawEllipse(QPointF(x, y), rr, rr);
        }
    } else if (f.element == "Water") {
        // 水柱：一串水滴 + 末端溅射
        const int N = 12;
        double dx = b.x() - a.x(), dy = b.y() - a.y();
        double len = std::sqrt(dx * dx + dy * dy);
        for (int i = 0; i < N; ++i) {
            double t = (i + (1.0 - k)) / static_cast<double>(N);
            if (t > 1.0) continue;
            double wob = std::sin(t * 9.0 + timeAcc * 12.0) * cell * 0.12;
            double x = a.x() + dx * t, y = a.y() + dy * t;
            if (len > 1e-6) { x += -dy / len * wob; y += dx / len * wob; }
            double rr = cell * 0.16 * (1.0 - t * 0.4);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(col.red(), col.green(), col.blue(), static_cast<int>(200 * k)));
            p.drawEllipse(QPointF(x, y), rr, rr);
            p.setBrush(QColor(255, 255, 255, static_cast<int>(160 * k)));
            p.drawEllipse(QPointF(x - rr * 0.25, y - rr * 0.25), rr * 0.4, rr * 0.4);
        }
    } else if (f.element == "Grass") {
        // 藤鞭：二次贝塞尔弧线 + 叶片
        double dx = b.x() - a.x(), dy = b.y() - a.y();
        double len = std::sqrt(dx * dx + dy * dy);
        QPointF ctrl((a.x() + b.x()) / 2, (a.y() + b.y()) / 2);
        if (len > 1e-6) { ctrl += QPointF(-dy / len, dx / len) * (cell * 1.1); }
        QPainterPath path(a);
        path.quadTo(ctrl, b);
        p.setPen(QPen(QColor(0x2E, 0x7D, 0x36, static_cast<int>(220 * k)),
                      cell * 0.16, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(path);
        p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), static_cast<int>(235 * k)),
                      cell * 0.09, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(path);
        p.setPen(Qt::NoPen);
        for (int i = 1; i < 5; ++i) {
            double t = i / 5.0;
            QPointF pt = path.pointAtPercent(t);
            p.setBrush(QColor(0x6D, 0xD0, 0x5A, static_cast<int>(220 * k)));
            p.drawEllipse(pt, cell * 0.12, cell * 0.07);
        }

    } else {
        // 兜底：普通光束
        p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), static_cast<int>(210 * k)),
                      3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(a, b);
    }
}

void GameWidget::drawDeployPreview(QPainter& p) {
    if (!game || deployType.empty() || !hovering) return;
    double cell = cellSize();
    QPointF c = gridToPixel(hoverCell.x(), hoverCell.y());
    bool occupied = !game->isDeployable(Position(hoverCell.x(), hoverCell.y()));

    std::unique_ptr<Pokemon> preview(createPokemonById(deployType));
    if(!preview)return;
    const int idx=static_cast<int>(preview->getType());
    const double rng=preview->getRange();
    const int cost=preview->getCost();
    const QString cls=QString::fromStdString(deployType);
    QColor col = typeColor(idx);

    bool afford = game->getPlayer().canAfford(static_cast<int>(cost));
    QColor ring = (occupied || !afford) ? QColor(0xFF, 0x55, 0x55) : QColor(0x6B, 0xFF, 0x9A);

    p.setBrush(QColor(ring.red(), ring.green(), ring.blue(), 40));
    p.setPen(QPen(ring, 2));
    p.drawRect(QRectF(c.x() - cell / 2, c.y() - cell / 2, cell, cell));

    p.setBrush(QColor(col.red(), col.green(), col.blue(), 34));
    p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), 150), 1, Qt::DashLine));
    p.drawEllipse(c, rng * cell, rng * cell);

    if (!occupied && afford) {
        p.setOpacity(0.75);
        if (!drawUnitSprite(p, cls, QPointF(c.x(), c.y()), std::max(32.0, cell * 1.85), false)) {
            QColor ghost = col; ghost.setAlpha(160);
            drawCreature(p, c, cell * 0.45, ghost, false, false);
        }
        p.setOpacity(1.0);
    }
}

void GameWidget::drawOverlay(QPainter& p) {
    if (!game) return;
    // 波次提示：加深色底衬，在草地上也能看清
    QRectF waveTag(12, 8, 96, 20);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x1b, 0x1b, 0x24, 215));
    p.drawRect(waveTag);
    p.setFont(uiFont(12, true));
    p.setPen(QColor(0xEE, 0xF5, 0xFF));
    p.drawText(waveTag, Qt::AlignCenter,
               QString("波次 %1/%2").arg(game->getWaveManager().getCurrentWave())
                                   .arg(game->getWaveManager().getMaxWave()));

    if (game->isPaused() && !game->isOver()) {
        p.fillRect(rect(), QColor(0, 0, 0, 90));
        p.setFont(uiFont(36, true));
        p.setPen(QColor(0xFF, 0xFF, 0xFF));
        p.drawText(rect(), Qt::AlignCenter, "已暂停");
    }

    if (game->isOver()) {
        p.fillRect(rect(), QColor(0, 0, 0, 165));
        p.setFont(uiFont(36, true));
        if (game->isVictory()) {
            p.setPen(QColor(0x6B, 0xFF, 0x9A));
            p.drawText(rect().adjusted(0, -20, 0, -20), Qt::AlignCenter, "胜  利");
        } else {
            p.setPen(QColor(0xFF, 0x6B, 0x6B));
            p.drawText(rect().adjusted(0, -20, 0, -20), Qt::AlignCenter, "核心已被摧毁");
        }
        p.setFont(uiFont(12, false));
        p.setPen(QColor(0xD8, 0xE6, 0xFF));
        p.drawText(rect().adjusted(0, 46, 0, 46), Qt::AlignCenter,
                   "点击右侧“重新开始”再来一局");
    }


}

void GameWidget::drawCreature(QPainter& p, const QPointF& c, double r,
                              const QColor& color, bool isEnemy, bool flash) {
    QColor body = flash ? QColor(255, 255, 255) : color;

    if (isEnemy) {
        QPolygonF shape;
        shape << QPointF(c.x(), c.y() - r)
              << QPointF(c.x() + r * 0.85, c.y())
              << QPointF(c.x(), c.y() + r)
              << QPointF(c.x() - r * 0.85, c.y());
        QRadialGradient g(c, r);
        g.setColorAt(0.0, body.lighter(150));
        g.setColorAt(1.0, body.darker(160));
        p.setBrush(g);
        p.setPen(QPen(QColor(255, 255, 255, 160), 1.5));
        p.drawPolygon(shape);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(20, 20, 30));
        p.drawEllipse(QPointF(c.x() - r * 0.28, c.y() - r * 0.05), r * 0.13, r * 0.13);
        p.drawEllipse(QPointF(c.x() + r * 0.28, c.y() - r * 0.05), r * 0.13, r * 0.13);
    } else {
        QPolygonF ears;
        ears << QPointF(c.x() - r * 0.6, c.y() - r * 0.6)
             << QPointF(c.x() - r * 0.95, c.y() - r * 1.5)
             << QPointF(c.x() - r * 0.2, c.y() - r * 0.85);
        p.setBrush(body.darker(120));
        p.setPen(Qt::NoPen);
        p.drawPolygon(ears);
        QPolygonF ears2;
        ears2 << QPointF(c.x() + r * 0.6, c.y() - r * 0.6)
              << QPointF(c.x() + r * 0.95, c.y() - r * 1.5)
              << QPointF(c.x() + r * 0.2, c.y() - r * 0.85);
        p.drawPolygon(ears2);

        QRadialGradient g(QPointF(c.x() - r * 0.3, c.y() - r * 0.3), r * 1.6);
        g.setColorAt(0.0, body.lighter(150));
        g.setColorAt(1.0, body.darker(150));
        p.setBrush(g);
        p.setPen(QPen(QColor(255, 255, 255, 180), 1.5));
        p.drawEllipse(c, r, r);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(25, 30, 45));
        p.drawEllipse(QPointF(c.x() - r * 0.32, c.y() - r * 0.08), r * 0.15, r * 0.18);
        p.drawEllipse(QPointF(c.x() + r * 0.32, c.y() - r * 0.08), r * 0.15, r * 0.18);
        p.setBrush(QColor(255, 255, 255));
        p.drawEllipse(QPointF(c.x() - r * 0.28, c.y() - r * 0.14), r * 0.05, r * 0.05);
        p.drawEllipse(QPointF(c.x() + r * 0.36, c.y() - r * 0.14), r * 0.05, r * 0.05);
    }
}

// ===================== 交互 =====================

void GameWidget::mouseMoveEvent(QMouseEvent* event) {
    hovering = true;
    hoverCell = gridFromPixel(QPointF(event->pos()));
    update();
}

void GameWidget::leaveEvent(QEvent* /*event*/) {
    hovering = false;
    update();
}

void GameWidget::mousePressEvent(QMouseEvent* event) {
    if (!game) return;
    if (!canvasRect().contains(QPointF(event->pos()))) return;
    QPoint cell = gridFromPixel(QPointF(event->pos()));

    if (event->button() == Qt::RightButton) {
        clearDeployType();
        setSelected(nullptr);
        return;
    }

    if (!deployType.empty()) {
        Pokemon* p = game->deployPokemon(deployType, Position(cell.x(), cell.y()));
        if (p) {
            clearDeployType();
            setSelected(p);
        }
        update();
        return;
    }

    double cs = cellSize();
    QPointF click = QPointF(event->pos());
    Pokemon* hit = nullptr;
    for (Pokemon* p : game->getMap().getDefenders()) {
        if (!p) continue;
        QPointF c = gridToPixel(p->getPosition().getX(), p->getPosition().getY());
        if (QRectF(c.x() - cs * 0.72, c.y() - cs * 2.0, cs * 1.44, cs * 2.25).contains(click)) { hit = p; break; }
    }
    setSelected(hit);
    update();
}
