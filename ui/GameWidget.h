#pragma once
// =============================================================
//  GameWidget.h
//  战斗地图渲染 + 交互
//  使用原生素材：绿宝石地面图块 + 宝可梦立绘 + 属性图标
//  普通攻击特效按元素类型区分
//  Qt 只负责"画"和"接收输入"，不承担属性计算 / 反应 / 波次等逻辑
// =============================================================
#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QPoint>
#include <QRectF>
#include <QString>
#include <QPixmap>
#include <QFont>
#include <QHash>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

class QPainter;
class Game;
class Pokemon;
class Enemy;

class GameWidget : public QWidget {
public:
    explicit GameWidget(QWidget* parent = nullptr);
    ~GameWidget() override;

    void setGame(Game* g);
    Game* getGame() const { return game; }
    void resetBattleVisuals();
    void tick(double dt);                  // 推进游戏 + 更新视觉特效

    void setDeployType(const std::string& type);
    void clearDeployType();
    std::string getDeployType() const { return deployType; }

    void setSelected(Pokemon* p);
    Pokemon* getSelected() const { return selected; }

    std::function<void(Pokemon*)> onSelectionChanged;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    // ---- 视觉特效 ----
    struct Particle {
        double x, y;        // 网格坐标
        double vx, vy;
        double life;
        double maxLife;
        QColor color;
        double size;
    };
    struct FloatingText {
        double x, y;
        QString text;
        double life;
        QColor color;
        bool big;
    };
    struct ReactionFx {
        QString name;
        double x, y;        // 网格坐标
        double life;
        double maxLife;
        QColor color;
    };
    struct AttackFlash {
        double x1, y1, x2, y2;
        double life;
        double maxLife;
        QColor color;
        QString element;      // 元素类型，决定普攻特效形态
    };

    // 坐标换算（含外圈树林边框）
    QRectF canvasRect() const;             // 实际战斗区域
    double cellSize() const;
    QPointF gridToPixel(double gx, double gy) const;
    QPoint gridFromPixel(const QPointF& p) const;

    QColor typeColor(int typeIndex) const;

    void computeRoadCells();

    void spawnBurst(double gx, double gy, const QColor& color, int count, double speed);
    void addText(double gx, double gy, const QString& text, const QColor& color, bool big = false, double duration = -1.0);
    void updateEffects(double dt);
    void detectCombatEffects();

    void drawBackground(QPainter& p);
    void drawPaths(QPainter& p);
    void drawCore(QPainter& p);
    void drawPokemon(QPainter& p);
    void drawEnemies(QPainter& p);
    void drawEffects(QPainter& p);
    void drawReactionEffect(QPainter& p, const ReactionFx& effect);
    void drawAttackEffect(QPainter& p, const AttackFlash& f);
    void drawDeployPreview(QPainter& p);
    void drawOverlay(QPainter& p);
    void drawCreature(QPainter& p, const QPointF& c, double r, const QColor& color,
                      bool isEnemy, bool flash);
    // 返回 false 表示没有素材，需要退化绘制
    // groundY 语义：脚底接地点（阴影中心）。targetH 是"可见内容"的高度。
    // 96x96 官方像素图四边有透明边距，按图片底边落地会让宝可梦悬空。
    bool drawUnitSprite(QPainter& p, const QString& className, const QPointF& ground,
                        double targetH, bool flash, bool friendly = false);

    // 素材缩放缓存（按像素尺寸缓存，避免每帧重缩放）
    QPixmap tileScaled(const QString& name, double cell);
    QPixmap spriteScaled(const QString& className, int heightPx, bool white);
    QRect spriteContentRect(const QString& className);   // 贴图不透明内容的包围盒（缓存）
    QHash<QString, QPixmap> tileCache;
    QHash<QString, QPixmap> spriteCache;
    QHash<QString, QRect> spriteRectCache;

    // 宝可梦风格血条（带黑描边 + 高光）；color 为空时按血量自动绿/黄/红
    void drawHpBar(QPainter& p, double x, double y, double w, double ratio,
                   const QColor& color = QColor());
    // 像素字体
    QFont uiFont(int px, bool bold) const;

    Game* game;
    std::string deployType;
    Pokemon* selected;

    std::vector<Particle> particles;
    int lastEvolveSeen = 0;             // 已播放过特效的进化次数
    std::vector<ReactionFx> reactionFx; // 元素反应特效（扩散环）
    std::vector<FloatingText> texts;
    std::vector<AttackFlash> flashes;

    std::unordered_map<const Enemy*, int> lastEnemyHp;

    std::vector<QPoint> roadCells;
    std::vector<QPoint> decoCells;
    bool roadReady;

    // 地表整张预渲染缓存（草变体/道路/树木/点缀一次画好，之后每帧只贴图）
    QPixmap terrainCache;
    int terrainCellPx;
    std::vector<char> roadFlag;      // GRID_W*GRID_H
    void rebuildTerrain();
    bool isRoadCell(int x, int y) const;

    bool hovering;
    QPoint hoverCell;
    double timeAcc;          // 用于脉冲动画
    double shakeTimer;
    double shakeMagnitude;
    unsigned subscriptionGeneration;
};
