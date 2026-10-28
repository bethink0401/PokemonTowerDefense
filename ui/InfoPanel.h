#pragma once
// =============================================================
//  InfoPanel.h
//  底部三个信息窗：当前单位(白) / 状态统计(白) / 战斗日志(蓝)
// =============================================================
#include <QWidget>
#include <functional>

class Game;
class Pokemon;
class QLabel;
class QPushButton;


class InfoPanel : public QWidget {
public:
    explicit InfoPanel(QWidget* parent = nullptr);

    void setGame(Game* g) { game = g; }
    void setSelectedPokemon(Pokemon* p);
    void refresh();
    QWidget* statusPanel() const { return statusFrame; }
    QWidget* logPanel() const { return logFrame; }

    std::function<void()> onUpgrade;
    std::function<void()> onSell;

private:
    QWidget* statusFrame;
    QWidget* logFrame;
    Game* game;
    Pokemon* selected;
    QLabel* spriteLabel;
    QLabel* unitName;
    QLabel* attackLabel;
    QLabel* upgradePreview;
    QLabel* unitValues[6];
    QLabel* statValues[8];
    QLabel* logLabel;
    QLabel* legendLabel;
    QLabel* evolutionImages[3];
    QLabel* evolutionNames[3];
    QLabel* evolutionLevels[3];
    QPushButton* upgradeBtn;
    QPushButton* sellBtn;
    QPushButton* strategyTabs[4];
};
