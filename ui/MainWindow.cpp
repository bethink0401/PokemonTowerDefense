#include "MainWindow.h"
#include "GameWidget.h"
#include "PokemonPanel.h"
#include "InfoPanel.h"
#include "AssetManager.h"
#include "Game.h"
#include "Player.h"
#include "WaveManager.h"
#include "GameMap.h"
#include "Core.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QElapsedTimer>
#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <initializer_list>

// 画一个宝可梦球图标（纯代码，不依赖素材）
static QPixmap makePokeball(int d) {
    QPixmap pm(d, d);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, false);
    QRectF r(0.5, 0.5, d - 1.0, d - 1.0);
    p.setPen(QPen(QColor(0x2b, 0x2b, 0x2b), 1));
    p.setBrush(QColor(0xee, 0x15, 0x15));
    p.drawChord(r, 0, 180 * 16);
    p.setBrush(QColor(0xf4, 0xf4, 0xf4));
    p.drawChord(r, 180 * 16, 180 * 16);
    p.setBrush(QColor(0x2b, 0x2b, 0x2b));
    p.drawRect(QRectF(r.left(), r.center().y() - 1, r.width(), 2));
    p.setBrush(QColor(0xf4, 0xf4, 0xf4));
    qreal bd = d * 0.36;
    p.drawEllipse(r.center(), bd / 2, bd / 2);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0x2b, 0x2b, 0x2b), 1));
    p.drawEllipse(r.center(), bd / 2, bd / 2);
    return pm;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      game(new Game()),
      gameWidget(nullptr),
      panel(nullptr),
      info(nullptr),
      timer(new QTimer(this)),
      clock(new QElapsedTimer()),
      ballLabel(nullptr),
      titleLabel(nullptr),
      goldLabel(nullptr),
      waveLabel(nullptr),
      coreLabel(nullptr),
      scoreLabel(nullptr) {
    // 先加载像素字体，再套样式表
    AssetManager::instance().fontFamily();
    setupUi();
    applyStyle();
    wireSignals();

    clock->start();
    timer->setInterval(16);            // 约 60 FPS
    timer->start();
    connect(timer, &QTimer::timeout, this, [this]() { onTick(); });
}

MainWindow::~MainWindow() {
    delete clock;
    delete game;
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    setWindowTitle("宝可梦动态策略塔防 · PokemonTowerDefense");
    resize(1380, 760);
    setMinimumSize(1100, 680);

    QVBoxLayout* root = new QVBoxLayout(central);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    // ---------- 顶部状态栏（蓝色窗口） ----------
    QFrame* top = new QFrame(central);
    top->setObjectName("winBlue");
    top->setFixedHeight(44);
    QHBoxLayout* topLay = new QHBoxLayout(top);
    topLay->setContentsMargins(0, 0, 0, 0);
    topLay->setSpacing(14);

    ballLabel = new QLabel(top);
    ballLabel->setPixmap(makePokeball(14));
    ballLabel->setFixedSize(14, 14);

    titleLabel = new QLabel("宝可梦动态策略塔防", top);
    titleLabel->setObjectName("appTitle");

    goldLabel = new QLabel(top);
    waveLabel = new QLabel(top);
    coreLabel = new QLabel(top);
    scoreLabel = new QLabel(top);
    for (QLabel* l : { goldLabel, waveLabel, coreLabel, scoreLabel }) {
        l->setObjectName("chip");
    }

    topLay->addWidget(ballLabel);
    topLay->addWidget(titleLabel);
    topLay->addStretch(1);
    topLay->addWidget(goldLabel);
    topLay->addWidget(waveLabel);
    topLay->addWidget(coreLabel);
    topLay->addWidget(scoreLabel);

    QWidget* body = new QWidget(central);
    QHBoxLayout* bodyLay = new QHBoxLayout(body);
    bodyLay->setContentsMargins(0, 0, 0, 0);
    bodyLay->setSpacing(6);
    QFrame* stage = new QFrame(body);
    stage->setObjectName("winBlue");
    QVBoxLayout* stageLay = new QVBoxLayout(stage);
    stageLay->setContentsMargins(0, 0, 0, 0);
    stageLay->setSpacing(0);
    gameWidget = new GameWidget(stage);
    gameWidget->setGame(game);
    stageLay->addWidget(gameWidget);
    panel = new PokemonPanel(body);
    panel->setGame(game);
    panel->setFixedWidth(330);
    QWidget* leftCol = new QWidget(body);
    QVBoxLayout* leftLay = new QVBoxLayout(leftCol);
    leftLay->setContentsMargins(0,0,0,0);
    leftLay->setSpacing(6);
    leftLay->addWidget(stage, 1);
    bodyLay->addWidget(leftCol, 1);
    bodyLay->addWidget(panel, 0);
    info = new InfoPanel(central);
    info->setGame(game);
    auto* sideLayout = static_cast<QVBoxLayout*>(panel->layout());
    sideLayout->insertWidget(sideLayout->count() - 1, info->statusPanel());
    sideLayout->insertWidget(sideLayout->count() - 1, info->logPanel(), 1);
    delete sideLayout->takeAt(sideLayout->count()-1);
    leftLay->addWidget(info);
    root->addWidget(top, 0);
    root->addWidget(body, 1);

    updateHud();
}

void MainWindow::wireSignals() {
    panel->onSelectPokemon = [this](const std::string& id) {
        std::string cur = gameWidget->getDeployType();
        if (cur == id) {
            gameWidget->clearDeployType();
            panel->syncSelection("");
        } else {
            gameWidget->setDeployType(id);
            panel->syncSelection(id);
        }
    };

    panel->onStart = [this]() {
        if (game->isOver() || !game->isRunning()) {
            gameWidget->setSelected(nullptr);
            info->setSelectedPokemon(nullptr);
            game->start();
            gameWidget->resetBattleVisuals();
        }
    };
    panel->onPauseToggle = [this]() {
        if (game->isPaused()) game->resume(); else game->pause();
    };
    panel->onRestart = [this]() {
        game->restart();
        gameWidget->resetBattleVisuals();
        gameWidget->clearDeployType();
        gameWidget->setSelected(nullptr);
        panel->syncSelection("");
        info->setSelectedPokemon(nullptr);
    };
    const QString savePath=QDir(QCoreApplication::applicationDirPath()).filePath("savegame.txt");
    panel->onSave = [this,savePath]() { game->save(savePath.toUtf8().toStdString()); };
    panel->onLoad = [this,savePath]() {
        const QString source=QFileInfo::exists(savePath)?savePath:QDir::current().filePath("savegame.txt");
        if(game->load(source.toUtf8().toStdString())) {
            gameWidget->setSelected(nullptr);
            info->setSelectedPokemon(nullptr);
            gameWidget->clearDeployType();panel->syncSelection("");
            gameWidget->resetBattleVisuals();
        }
    };

    gameWidget->onSelectionChanged = [this](Pokemon* p) {
        info->setSelectedPokemon(p);
    };

    info->onUpgrade = [this]() {
        Pokemon* sel = gameWidget->getSelected();
        if (game->upgradePokemon(sel)) {
            // 进化会把旧形态对象整体替换，因此要把新指针同步回视图层
            gameWidget->setSelected(sel);
            info->setSelectedPokemon(sel);
        }
        info->refresh();
    };
    info->onSell = [this]() {
        game->sellPokemon(gameWidget->getSelected());
        gameWidget->setSelected(nullptr);
        info->setSelectedPokemon(nullptr);
    };
}

void MainWindow::onTick() {
    qint64 ms = clock->restart();
    double dt = ms / 1000.0;
    if (dt > 0.05) dt = 0.05;      // 防止卡顿导致穿透
    if (dt < 0.0) dt = 0.0;

    gameWidget->tick(dt);
    panel->refresh();
    panel->syncSelection(gameWidget->getDeployType());
    info->refresh();
    updateHud();
}

void MainWindow::updateHud() {
    goldLabel->setText(QString("金币 %1").arg(game->getPlayer().getGold()));
    waveLabel->setText(QString("波次 %1 / %2")
                           .arg(game->getWaveManager().getCurrentWave())
                           .arg(game->getWaveManager().getMaxWave()));
    coreLabel->setText(QString("核心 %1 / %2")
                           .arg(game->getMap().getCore().getHp())
                           .arg(game->getMap().getCore().getMaxHp()));
    scoreLabel->setText(QString("分数 %1").arg(game->getPlayer().getScore()));
}

void MainWindow::applyStyle() {
    const QString dir = AssetManager::instance().dir();
    const QString fam = "Microsoft YaHei";
    const QString famCss = fam.isEmpty() ? QString("Microsoft YaHei") : fam;

    QString whiteWin, blueWin;
    if (!dir.isEmpty()) {
        // 绿宝石原生对话框边框：九宫格拉伸（切片 16，对应 2x 素材）
        whiteWin = QString("#winWhite { background:#f8fbff; border-image: url(%1/ui/window_white.png) 16 16 16 16 repeat repeat;"
                           " border-width:8px; border-style:solid; }").arg(dir);
        blueWin = QString("#winBlue { background:#164b92; border-image: url(%1/ui/window_blue.png) 16 16 16 16 repeat repeat;"
                          " border-width:8px; border-style:solid; }").arg(dir);
    } else {
        whiteWin = "#winWhite { background:#e6e6ee; border:2px solid #1862c5; }";
        blueWin = "#winBlue { background:#1862c5; border:2px solid #0e3f86; }";
    }

    QString qss = QString(R"QSS(
        QMainWindow { background:#DCE8F6; color:#172b42; }
        QWidget { color:#172b42; }
        QLabel { background:transparent; }
        QMainWindow { background-image:none; }
        QLabel { font-family:"%1"; font-size:14px; }

        %2
        %3

        QFrame#winWhite QLabel, QFrame#winWhite { color:#383838; }
        QFrame#winBlue  QLabel, QFrame#winBlue  { color:#ffffff; }

        QLabel#appTitle { color:#ffffff; }
        QLabel#chip { color:#ffffff; }

        QLabel#cap {
            background:#1862c5; color:#f0f0f8; padding:0 6px;
            font-family:"%1"; font-size:14px;
        }
        QFrame#winWhite QLabel#cap { color:#ffffff; font-weight:bold; }
        QPushButton#strategyTab { background:#fffbea; color:#263a50; border:2px solid #1c4b83; border-radius:0; padding:2px 3px; font-family:"%1"; font-size:12px; }
        QPushButton#strategyTab:hover { background:#fff1b5; }
        QPushButton#strategyTab:checked { background:#ffe58c; color:#263a50; border-color:#a97423; font-weight:bold; }
        QPushButton#strategyTab:disabled { background:#e6ebf0; color:#778797; border-color:#a2b2c2; }
        QLabel#capGold {
            background:#e8c840; color:#3a2c00; padding:0 6px;
            font-family:"%1"; font-size:14px; font-weight:bold;
        }
        QLabel#infoText { color:#383838; font-family:"%1"; font-size:14px; }
        QFrame#winBlue QLabel#infoText { color:#ffffff; }
        QLabel#hint { color:#41546b; font-family:"%1"; font-size:14px; }
        QFrame#winBlue QLabel#hint { color:#e7efff; }

        QPushButton#entry {
            background:#e3edf9; border:1px solid #a6bddb; border-radius:4px; text-align:left;
            padding:5px 6px; color:#203950;
            font-family:"%1"; font-size:14px;
        }
        QPushButton#entry:hover { background:rgba(24,98,197,36); }
        QPushButton#entry:checked { background:#f6e07a; color:#383838; }
        QPushButton#entry:disabled { color:#65758b; }

        QPushButton#entryLight {
            font-weight:bold;
            background:#2868b2; border:1px solid #7db1ed; border-radius:4px; text-align:left;
            padding:5px 6px; color:#ffffff;
            font-family:"%1"; font-size:14px;
        }
        QPushButton#entryLight:hover { background:rgba(255,255,255,40); }
        QPushButton#entryLight:disabled { color:#c0cfdf; }

        QScrollBar:vertical { background:#C6D8EE; width:8px; }
        QScrollBar::handle:vertical { background:#8FA9C8; }
    )QSS").arg(famCss, whiteWin, blueWin);

    setStyleSheet(qss);

    QFont f = QApplication::font();
    if (!fam.isEmpty()) f.setFamily(fam);
    f.setPixelSize(14);
    qApp->setFont(f);
}

