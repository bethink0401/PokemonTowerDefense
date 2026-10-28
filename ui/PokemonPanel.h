#pragma once
// =============================================================
//  PokemonPanel.h
//  右侧：部署窗口（白底） + 指令窗口（蓝底），GBA 走马灯式菜单项
// =============================================================
#include <QWidget>
#include <string>
#include <vector>
#include <functional>

class Game;
class QPushButton;
class QLabel;

class PokemonPanel : public QWidget {
public:
    explicit PokemonPanel(QWidget* parent = nullptr);

    void setGame(Game* g) { game = g; }
    void refresh();
    void syncSelection(const std::string& deployType);

    std::function<void(const std::string&)> onSelectPokemon;
    std::function<void()> onStart;
    std::function<void()> onPauseToggle;
    std::function<void()> onSave;
    std::function<void()> onLoad;
    std::function<void()> onRestart;

private:
    Game* game;
    std::vector<QPushButton*> pokeButtons;
    std::vector<std::string> pokeIds;
    QPushButton* startBtn;
    QPushButton* pauseBtn;
    QPushButton* saveBtn;
    QPushButton* loadBtn;
    QPushButton* restartBtn;
};
