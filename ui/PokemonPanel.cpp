#include "PokemonPanel.h"
#include "Game.h"
#include "Player.h"
#include "WaveManager.h"
#include "AssetManager.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <cstddef>

// 按目标高度等比换算宽度
static int fitWidth(const QPixmap& pm, int h) {
    if (pm.isNull() || pm.height() == 0) return 0;
    return static_cast<int>(pm.width() * (h / static_cast<double>(pm.height())));
}

// 把宝可梦立绘与属性徽章拼成按钮图标
static QPixmap composeEntryIcon(const QString& className, const QString& typeName, int h) {
    QPixmap face = AssetManager::instance().pokemon(className);
    QImage image = face.toImage();
    QRect bounds;
    for (int y=0; y<image.height(); ++y)
        for (int x=0; x<image.width(); ++x)
            if (image.pixelColor(x,y).alpha() > 8) bounds |= QRect(x,y,1,1);
    if (!bounds.isEmpty()) face = face.copy(bounds);
    QPixmap out(h, h);
    out.fill(Qt::transparent);
    QPainter p(&out);
    if (!face.isNull()) {
        QPixmap f = face.scaled(h, h, Qt::KeepAspectRatio, Qt::FastTransformation);
        p.drawPixmap((h-f.width())/2, h-f.height(), f);
    }
    return out;
}

PokemonPanel::PokemonPanel(QWidget* parent)
    : QWidget(parent), game(nullptr) {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    // ---------------- 部署窗口（白底） ----------------
    QFrame* deployFrame = new QFrame(this);
    deployFrame->setObjectName("winWhite");
    QVBoxLayout* dLay = new QVBoxLayout(deployFrame);
    dLay->setContentsMargins(0, 0, 0, 0);
    dLay->setSpacing(2);

    QLabel* cap1 = new QLabel("部署宝可梦", deployFrame);
    cap1->setObjectName("cap");
    cap1->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    dLay->addWidget(cap1);

    pokeIds = { "Pichu", "Charmander", "Squirtle", "Bulbasaur" };
    struct Info { const char* disp; const char* typeKey; };
    const Info infos[4] = {
        { "皮丘",   "Electric" },
        { "小火龙",   "Fire" },
        { "杰尼龟",   "Water" },
        { "妙蛙种子", "Grass" }
    };

    QGridLayout* choices = new QGridLayout();
    choices->setSpacing(6);
    dLay->addLayout(choices);
    for (int i = 0; i < 4; ++i) {
        QPushButton* b = new QPushButton(deployFrame);
        b->setObjectName("entry");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        QPixmap ic = composeEntryIcon(QString::fromUtf8(pokeIds[i].c_str()),
                                      QString::fromUtf8(infos[i].typeKey), 44);
        if (!ic.isNull()) {
            b->setIcon(QIcon(ic));
            b->setIconSize(ic.size());
        }
        b->setText(QString("%1\n%2 金币")
                       .arg(QString::fromUtf8(infos[i].disp))
                       .arg(Game::deployCost(pokeIds[i])));
        const char* roles[]={"电系：攻速成长突出，概率追击，擅长单体输出。","火系：攻击与溅射成长突出，擅长清理密集敌群。","水系：减速随等级和进化增强，擅长配合队友控制道路。","草系：攻击与射程成长突出，进化提高破甲，擅长重甲目标。"};
        b->setToolTip(QString::fromUtf8(roles[i]));
        const std::string id = pokeIds[i];
        connect(b, &QPushButton::clicked, this, [this, id]() {
            if (onSelectPokemon) onSelectPokemon(id);
        });
        pokeButtons.push_back(b);
        choices->addWidget(b, i / 2, i % 2);
    }

    QLabel* hint = new QLabel("仅标记点位可部署 · 右键取消", deployFrame);
    hint->setObjectName("hint");
    hint->setWordWrap(true);
    dLay->addWidget(hint);

    root->addWidget(deployFrame);

    // ---------------- 指令窗口（蓝底） ----------------
    QFrame* cmdFrame = new QFrame(this);
    cmdFrame->setObjectName("winBlue");
    QVBoxLayout* cLay = new QVBoxLayout(cmdFrame);
    cLay->setContentsMargins(0, 0, 0, 0);
    cLay->setSpacing(2);

    QLabel* cap2 = new QLabel("指令", cmdFrame);
    cap2->setObjectName("capGold");
    cap2->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    cLay->addWidget(cap2);

    QGridLayout* commands = new QGridLayout();
    commands->setSpacing(6);
    cLay->addLayout(commands);
    int commandIndex = 0;
    auto makeEntry = [&](const char* text) {
        QPushButton* b = new QPushButton(cmdFrame);
        b->setObjectName("entryLight");
        b->setCursor(Qt::PointingHandCursor);
        b->setText(QString("%1").arg(QString::fromUtf8(text)));
        commands->addWidget(b, commandIndex / 2, commandIndex % 2);
        ++commandIndex;
        return b;
    };

    startBtn   = makeEntry("开始 / 下一波");
    pauseBtn   = makeEntry("暂停");
    restartBtn = makeEntry("重新开始");
    saveBtn    = makeEntry("保存进度");
    loadBtn    = makeEntry("读取进度");

    startBtn->setIcon(QIcon(AssetManager::instance().actionIcon("start")));
    startBtn->setIconSize(QSize(20,20));
    pauseBtn->setIcon(QIcon(AssetManager::instance().actionIcon("pause")));
    pauseBtn->setIconSize(QSize(20,20));
    restartBtn->setIcon(QIcon(AssetManager::instance().actionIcon("restart")));
    restartBtn->setIconSize(QSize(20,20));
    saveBtn->setIcon(QIcon(AssetManager::instance().actionIcon("save")));
    saveBtn->setIconSize(QSize(20,20));
    loadBtn->setIcon(QIcon(AssetManager::instance().actionIcon("load")));
    loadBtn->setIconSize(QSize(20,20));
    connect(startBtn, &QPushButton::clicked, this, [this]() { if (onStart) onStart(); });
    connect(pauseBtn, &QPushButton::clicked, this, [this]() { if (onPauseToggle) onPauseToggle(); });
    connect(restartBtn, &QPushButton::clicked, this, [this]() { if (onRestart) onRestart(); });
    connect(saveBtn, &QPushButton::clicked, this, [this]() { if (onSave) onSave(); });
    connect(loadBtn, &QPushButton::clicked, this, [this]() { if (onLoad) onLoad(); });

    root->addWidget(cmdFrame);
    root->addStretch(1);
}

void PokemonPanel::refresh() {
    if (!game) return;
    bool over = game->isOver();
    int gold = game->getPlayer().getGold();
    for (size_t i = 0; i < pokeButtons.size(); ++i) {
        pokeButtons[i]->setEnabled(!over && gold >= Game::deployCost(pokeIds[i]));
    }
    startBtn->setEnabled(!over && !game->getWaveManager().isWaveActive());
    pauseBtn->setText(QString("%1").arg(game->isPaused() ? "继续" : "暂停"));
    pauseBtn->setEnabled(!over);
    pauseBtn->setIcon(QIcon(AssetManager::instance().actionIcon(game->isPaused() ? "start" : "pause")));
}

void PokemonPanel::syncSelection(const std::string& deployType) {
    for (size_t i = 0; i < pokeButtons.size(); ++i) {
        pokeButtons[i]->setChecked(pokeIds[i] == deployType);
    }
}
