#pragma once
// =============================================================
//  MainWindow.h
//  主窗口：GBA 宝可梦风格界面
//   顶部状态栏(蓝窗) + 战斗地图(蓝窗) + 右侧部署/指令栏 + 底部三个信息窗
//  Qt 只负责界面与驱动 QTimer，不承担任何战斗逻辑
// =============================================================
#include <QMainWindow>
#include <QString>

class Game;
class GameWidget;
class PokemonPanel;
class InfoPanel;
class QLabel;
class QTimer;
class QElapsedTimer;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void applyStyle();
    void wireSignals();
    void onTick();
    void updateHud();

    Game* game;
    GameWidget* gameWidget;
    PokemonPanel* panel;
    InfoPanel* info;
    QTimer* timer;
    QElapsedTimer* clock;

    QLabel* ballLabel;
    QLabel* titleLabel;
    QLabel* goldLabel;
    QLabel* waveLabel;
    QLabel* coreLabel;
    QLabel* scoreLabel;
};
