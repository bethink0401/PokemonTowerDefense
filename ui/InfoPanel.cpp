#include "InfoPanel.h"
#include "Game.h"
#include "Pokemon.h"
#include "UnitInsights.h"
#include <memory>
#include "TargetStrategy.h"
#include "Statistics.h"
#include "AssetManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QStringList>
#include <vector>
#include <QGridLayout>
#include <QIcon>
#include <QButtonGroup>

InfoPanel::InfoPanel(QWidget* parent)
    : QWidget(parent), game(nullptr), selected(nullptr) {
    setFixedHeight(214);

    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    // Selected-unit card: persistent strategy choices beside the name.
    QFrame* f1 = new QFrame(this);
    f1->setObjectName("winWhite");
    f1->setProperty("panelRole","selectedUnit");
    QVBoxLayout* l1 = new QVBoxLayout(f1);
    l1->setContentsMargins(0,0,0,0);
    l1->setSpacing(4);
    QLabel* cap1=new QLabel("当前单位",f1);
    cap1->setObjectName("cap");
    cap1->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Fixed);
    l1->addWidget(cap1);

    QHBoxLayout* heading=new QHBoxLayout();
    heading->setSpacing(6);
    unitName=new QLabel("未选中宝可梦",f1);
    unitName->setObjectName("unitName");
    unitName->setFixedWidth(180);
    unitName->setStyleSheet("font-size:15px;font-weight:bold;color:#193c5d;");
    heading->addWidget(unitName);
    QLabel* strategyCaption=new QLabel("索敌",f1);
    strategyCaption->setObjectName("infoText");
    heading->addWidget(strategyCaption);
    auto* strategies=new QButtonGroup(f1);
    strategies->setExclusive(true);
    const char* ids[]={"Nearest","LowestHP","ClosestToCore","Fastest"};
    const char* labels[]={"最近目标","最低生命","最接近核心","最快敌人"};
    const char* icons[]={"strategy_nearest","strategy_lowesthp","strategy_closesttocore","strategy_fastest"};
    for(int i=0;i<4;++i) {
        auto* button=new QPushButton(QString::fromUtf8(labels[i]),f1);
        button->setObjectName("strategyTab");
        button->setProperty("strategyId",ids[i]);
        button->setCheckable(true);
        button->setEnabled(false);
        button->setIcon(QIcon(AssetManager::instance().actionIcon(icons[i])));
        button->setIconSize(QSize(16,16));
        button->setFixedSize(i==2?104:88,28);
        strategies->addButton(button,i);
        heading->addWidget(button);
        strategyTabs[i]=button;
        const std::string id=ids[i];
        connect(button,&QPushButton::clicked,this,[this,id](){
            if(game && selected) game->setPokemonStrategy(selected,id);
            refresh();
        });
    }
    heading->addStretch(1);
    l1->addLayout(heading);

    QHBoxLayout* body=new QHBoxLayout();
    body->setSpacing(8);
    spriteLabel=new QLabel(f1);
    spriteLabel->setFixedSize(72,72);
    spriteLabel->setAlignment(Qt::AlignCenter);
    body->addWidget(spriteLabel);
    QVBoxLayout* details=new QVBoxLayout();
    details->setSpacing(3);
    QGridLayout* unitGrid=new QGridLayout();
    unitGrid->setSpacing(4);
    const char* unitCaptions[]={"预计单体秒伤","射程","出手间隔","下次出手","范围内敌人","累计伤害"};
    for(int i=0;i<6;++i) {
        QFrame* cell=new QFrame(f1);
        cell->setObjectName("unitMetric");
        cell->setStyleSheet(QString("QFrame#unitMetric {background:#fffbea;border-image:url(%1/ui/window_white.png) 16 16 16 16 stretch stretch;border-width:3px;border-style:solid;border-radius:0;}").arg(AssetManager::instance().dir()));
        QVBoxLayout* content=new QVBoxLayout(cell);
        content->setContentsMargins(3,2,3,2);content->setSpacing(0);
        unitValues[i]=new QLabel("—",cell);
        unitValues[i]->setObjectName(QString("unitValue%1").arg(i));
        unitValues[i]->setAlignment(Qt::AlignCenter);
        unitValues[i]->setStyleSheet("font-size:15px;font-weight:bold;color:#193c5d;background:transparent;");
        QLabel* caption=new QLabel(QString::fromUtf8(unitCaptions[i]),cell);
        caption->setAlignment(Qt::AlignCenter);
        caption->setStyleSheet("font-size:12px;color:#41546b;background:transparent;");
        content->addWidget(unitValues[i]);content->addWidget(caption);
        unitGrid->addWidget(cell,i/3,i%3);unitGrid->setColumnStretch(i%3,1);
    }
    unitValues[0]->setToolTip("基准秒伤 = 面板攻击 ÷ 攻击间隔；未计克制、护甲、概率追击、持续伤害与元素反应。");
    unitValues[2]->setToolTip("攻击间隔直接决定攻击频率；升级与进化可缩短间隔。");
    unitValues[3]->setToolTip("攻击间隔结束后可再次出手；没有敌人进入范围时保持就绪。暂停时倒计时停止。");
    unitValues[5]->setToolTip("累计实际扣血：攻击、溅射、概率追击和该次攻击触发的即时反应；不含后续持续伤害。进化与存读档后保留。");
    details->addLayout(unitGrid);
    attackLabel=new QLabel("点选地图上的我方宝可梦查看详情",f1);
    attackLabel->setObjectName("unitAttack");
    attackLabel->setWordWrap(false);
    attackLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    attackLabel->setStyleSheet("font-size:12px;color:#304a63;");
    details->addWidget(attackLabel);
    body->addLayout(details,1);

    QHBoxLayout* actions=new QHBoxLayout();actions->setSpacing(4);
    upgradeBtn=new QPushButton("升级\n—",f1);sellBtn=new QPushButton("出售\n—",f1);
    upgradeBtn->setObjectName("entry");sellBtn->setObjectName("entry");
    upgradeBtn->setFixedWidth(100);sellBtn->setFixedWidth(100);
    upgradeBtn->setIcon(QIcon(AssetManager::instance().actionIcon("upgrade")));
    sellBtn->setIcon(QIcon(AssetManager::instance().actionIcon("sell")));
    upgradeBtn->setIconSize(QSize(20,20));sellBtn->setIconSize(QSize(20,20));
    actions->addWidget(upgradeBtn);actions->addWidget(sellBtn);

    QWidget* evolutionBox=new QWidget(f1);
    evolutionBox->setObjectName("evolutionBox");evolutionBox->setFixedWidth(224);
    QVBoxLayout* evolution=new QVBoxLayout(evolutionBox);
    evolution->setContentsMargins(0,0,0,0);evolution->setSpacing(2);
    evolution->addLayout(actions);
    upgradePreview=new QLabel(evolutionBox);upgradePreview->setObjectName("upgradePreview");
    upgradePreview->setStyleSheet("font-size:12px;color:#304a63;");
    upgradePreview->setWordWrap(false);upgradePreview->setFixedHeight(18);
    evolution->addWidget(upgradePreview);
    QHBoxLayout* chain=new QHBoxLayout();chain->setSpacing(3);
    for(int i=0;i<3;++i) {
        if(i) {
            QLabel* arrow=new QLabel("→",evolutionBox);arrow->setObjectName("infoText");
            arrow->setFixedSize(14,72);arrow->setAlignment(Qt::AlignCenter);
            chain->addWidget(arrow,0,Qt::AlignTop);
        }
        QVBoxLayout* stage=new QVBoxLayout();stage->setSpacing(1);stage->setAlignment(Qt::AlignTop);
        evolutionLevels[i]=new QLabel(evolutionBox);
        evolutionLevels[i]->setObjectName(QString("evolutionLevel%1").arg(i));
        evolutionLevels[i]->setFixedHeight(16);evolutionLevels[i]->setAlignment(Qt::AlignCenter);
        evolutionImages[i]=new QLabel(evolutionBox);evolutionImages[i]->setFixedSize(40,40);evolutionImages[i]->setAlignment(Qt::AlignCenter);
        evolutionNames[i]=new QLabel(evolutionBox);evolutionNames[i]->setObjectName("infoText");evolutionNames[i]->setFixedHeight(18);evolutionNames[i]->setAlignment(Qt::AlignCenter);
        stage->addWidget(evolutionLevels[i]);stage->addWidget(evolutionImages[i],0,Qt::AlignHCenter);stage->addWidget(evolutionNames[i]);
        chain->addLayout(stage);
    }
    evolution->addLayout(chain);
    body->addWidget(evolutionBox,0,Qt::AlignRight);
    l1->addLayout(body,1);
    connect(upgradeBtn,&QPushButton::clicked,this,[this](){if(onUpgrade)onUpgrade();});
    connect(sellBtn,&QPushButton::clicked,this,[this](){if(onSell)onSell();});

    // ---------------- 状态 / 统计（白底） ----------------
    QFrame* f2 = new QFrame(this);
    statusFrame = f2;
    f2->setObjectName("winWhite");
    QVBoxLayout* l2 = new QVBoxLayout(f2);
    l2->setContentsMargins(0, 0, 0, 0);
    l2->setSpacing(2);

    QLabel* cap2 = new QLabel("状态统计", f2);
    cap2->setObjectName("cap");
    cap2->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    l2->addWidget(cap2);

    QGridLayout* metrics = new QGridLayout();
    metrics->setSpacing(4);
    const char* captions[] = {"敌人","防守","核心","战况","击杀","元素反应","漏怪","累计金币"};
    for(int i=0;i<8;++i) {
        QFrame* cell=new QFrame(f2);
        cell->setObjectName("statCell");
        cell->setStyleSheet(QString("QFrame#statCell {background:#fffbea;border-image:url(%1/ui/window_white.png) 16 16 16 16 stretch stretch;border-width:3px;border-style:solid;border-radius:0;}").arg(AssetManager::instance().dir()));
        QVBoxLayout* cellLayout=new QVBoxLayout(cell);
        cellLayout->setContentsMargins(2,3,2,3);
        cellLayout->setSpacing(1);
        statValues[i]=new QLabel("0",cell);
        statValues[i]->setObjectName(QString("statValue%1").arg(i));
        statValues[i]->setAlignment(Qt::AlignCenter);
        statValues[i]->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        statValues[i]->setStyleSheet("font-size:16px;font-weight:bold;color:#163e61;background:transparent;");
        QLabel* caption=new QLabel(QString::fromUtf8(captions[i]),cell);
        caption->setAlignment(Qt::AlignCenter);
        caption->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        caption->setStyleSheet("font-size:12px;color:#41546b;background:transparent;");
        cellLayout->addWidget(statValues[i]);
        cellLayout->addWidget(caption);
        cell->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        metrics->addWidget(cell,i/4,i%4);
        metrics->setColumnStretch(i%4,1);
        metrics->setRowStretch(i/4,1);
    }
    l2->addLayout(metrics,1);

    // ---------------- 战斗日志（蓝底） ----------------
    QFrame* f3 = new QFrame(this);
    logFrame = f3;
    f3->setObjectName("winBlue");
    QVBoxLayout* l3 = new QVBoxLayout(f3);
    l3->setContentsMargins(0, 0, 0, 0);
    l3->setSpacing(2);

    QLabel* cap3 = new QLabel("战斗日志", f3);
    cap3->setObjectName("capGold");
    cap3->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    l3->addWidget(cap3);

    logLabel = new QLabel("-", f3);
    logLabel->setObjectName("infoText");
    logLabel->setWordWrap(true);
    logLabel->setStyleSheet("font-weight:bold;");
    legendLabel = new QLabel(f3);
    legendLabel->setObjectName("hint");
    legendLabel->setWordWrap(true);
    l3->addWidget(logLabel);
    legendLabel->hide();
    cap3->setToolTip("元素反应：火+水蒸汽 / 水+电电导 / 火+草燃烧 / 水+草缠绕 / 火+电爆破 / 电+草孢子；麻痹使目标随机短暂停步");
    l3->addStretch(1);

    legendLabel->setText("> 元素反应：火+水蒸汽 水+电电导 电+草孢子\n"
                         "> 火+草燃烧 水+草缠绕 火+电爆破");

    root->addWidget(f1, 1);


}

void InfoPanel::setSelectedPokemon(Pokemon* p) {
    selected = p;
    refresh();
}

void InfoPanel::refresh() {
    if (!game) return;

    static const char* families[4][3] = {
        {"Pichu","Pikachu","Raichu"}, {"Charmander","Charmeleon","Charizard"},
        {"Squirtle","Wartortle","Blastoise"}, {"Bulbasaur","Ivysaur","Venusaur"}
    };
    int family=0;
    if (selected)
        for (int f=0; f<4; ++f)
            for (int i=0; i<3; ++i)
                if (selected->getClassName()==families[f][i]) family=f;
    for (int i=0; i<3; ++i) {
        QString id=families[family][i];
        QPixmap sprite=AssetManager::instance().pokemon(id);
        QImage image=sprite.toImage();
        QRect bounds;
        for(int y=0;y<image.height();++y) for(int x=0;x<image.width();++x)
            if(image.pixelColor(x,y).alpha()>8) bounds|=QRect(x,y,1,1);
        if(!bounds.isEmpty()) sprite=sprite.copy(bounds);
        evolutionImages[i]->setPixmap(sprite.scaled(38,38,Qt::KeepAspectRatio,Qt::FastTransformation));
        evolutionNames[i]->setText(QString::fromStdString(pokemonDisplayName(families[family][i])));
        int requiredLevel=i==0?1:i==1?3:5;
        evolutionLevels[i]->setText(QString("Lv.%1").arg(requiredLevel));
        evolutionLevels[i]->setToolTip(i==0 ? "初始形态" : QString("达到 %1 级后进化为此形态").arg(requiredLevel));
        bool current=selected && selected->getStage()==i;
        evolutionLevels[i]->setStyleSheet(current ? "font-size:12px;font-weight:bold;color:#125537;background:#c9f5d5;" : "font-size:12px;color:#61758b;");
        evolutionNames[i]->setStyleSheet(current ? "color:#125537;background:#c9f5d5;font-weight:bold;border-radius:3px;" : "color:#54677d;");
    }
    const char* ids[]={"Nearest","LowestHP","ClosestToCore","Fastest"};
    if(selected) {
        QPixmap face=AssetManager::instance().pokemon(QString::fromStdString(selected->getClassName()));
        QImage image=face.toImage();QRect bounds;
        for(int y=0;y<image.height();++y) for(int x=0;x<image.width();++x)
            if(image.pixelColor(x,y).alpha()>8) bounds|=QRect(x,y,1,1);
        if(!bounds.isEmpty())face=face.copy(bounds);
        spriteLabel->setPixmap(face.scaled(70,70,Qt::KeepAspectRatio,Qt::FastTransformation));
        unitName->setText(QString("%1 Lv.%2").arg(QString::fromStdString(pokemonDisplayName(selected->getClassName()))).arg(selected->getLevel()));
        const double remaining=UnitInsights::remaining(*selected);
        const QString values[]={QString::number(UnitInsights::dps(*selected),'f',1),
            QString::number(selected->getRange(),'f',1)+" 格",QString::number(UnitInsights::interval(*selected),'f',2)+" 秒",
            remaining>0?QString("%1 秒").arg(remaining,0,'f',1):QString("就绪"),QString::number(UnitInsights::enemiesInRange(*selected,game->getMap())),QString::number(selected->getCumulativeDamage())};
        for(int i=0;i<6;++i)unitValues[i]->setText(values[i]);
        const QString stage=selected->getStage()==0?"初形态":selected->getStage()==1?"一阶进化":"最终形态";
        const QString effect=selected->getType()==PokemonType::Fire?"群伤·灼烧":selected->getType()==PokemonType::Water?"稳定减速":selected->getType()==PokemonType::Electric?"高速追击":"远程破甲";
        attackLabel->setText(QString("普通攻击 · %1 · %2").arg(effect,stage));
        QString trait;
        switch(selected->getType()) {
        case PokemonType::Electric: trait="偏重攻速成长；20%概率追击，额外伤害4+等级。"; break;
        case PokemonType::Fire: trait=QString("偏重攻击与范围群伤；溅射半径%1格，比例%2%；灼烧2秒。").arg(selected->getSplashRadius(),0,'f',1).arg(selected->getSplashRatio()*100,0,'f',0); break;
        case PokemonType::Water: trait=QString("偏重稳定控制；减速%1%，持续%2秒。").arg(selected->getSlowRatio()*100,0,'f',0).arg(selected->getSlowDuration(),0,'f',2); break;
        case PokemonType::Grass: trait=QString("偏重攻击与射程；普通攻击忽略%1%的护甲减伤。").arg(selected->getArmorPenetration()*100,0,'f',0); break;
        }
        attackLabel->setToolTip(QString("%1属性 · 攻击力 %2\n%3\n普通攻击施加自身元素，攻击间隔决定出手频率。").arg(QString::fromStdString(typeDisplayName(selected->getType()))).arg(selected->getAttackPower()).arg(trait));
        unitName->setText(QString("%1 Lv.%2 · %3").arg(QString::fromStdString(pokemonDisplayName(selected->getClassName()))).arg(selected->getLevel()).arg(QString::fromStdString(typeDisplayName(selected->getType()))));
        const std::string active=selected->getStrategy()?selected->getStrategy()->getName():"Nearest";
        for(int i=0;i<4;++i){strategyTabs[i]->setEnabled(true);strategyTabs[i]->setChecked(active==ids[i]);}
        const bool maxed=selected->getLevel()>=5;
        upgradeBtn->setText(maxed ? "已满级\nLv.5" : QString("%1\n%2 金币").arg(selected->getLevel()==2||selected->getLevel()==4?"进化":"升级").arg(selected->getUpgradeCost()));
        if(maxed){upgradePreview->setText("最终培养完成 · 可调整索敌策略");upgradePreview->setToolTip("已达到 Lv.5，无法继续升级。");}
        else {
            std::unique_ptr<Pokemon> next(createPokemonById(selected->getClassName()));next->setLevel(selected->getLevel()+1);
            if(next->canEvolve())next.reset(next->createEvolved());
            QStringList improvements;
            if(next->getAttackPower()>selected->getAttackPower())improvements<<"攻击提高";
            if(next->getRange()>selected->getRange())improvements<<"射程扩大";
            if(UnitInsights::interval(*next)<UnitInsights::interval(*selected))improvements<<"出手更快";
            if(selected->getType()==PokemonType::Fire && next->getSplashRadius()>selected->getSplashRadius())improvements<<"溅射增强";
            if(selected->getType()==PokemonType::Water && next->getSlowRatio()>selected->getSlowRatio())improvements<<"减速增强";
            if(selected->getType()==PokemonType::Grass && next->getArmorPenetration()>selected->getArmorPenetration())improvements<<"破甲增强";
            const QString full=improvements.join(" · ");
            if(improvements.size()>3)improvements.removeAt(2);
            upgradePreview->setText(improvements.join(" · "));
            upgradePreview->setToolTip((next->getStage()>selected->getStage()?QString("进化为%1：").arg(QString::fromStdString(pokemonDisplayName(next->getClassName()))):QString("升级后："))+full);
        }
        upgradeBtn->setEnabled(!maxed && game->getPlayer().canAfford(selected->getUpgradeCost()));
        sellBtn->setText(QString("出售\n%1 金币").arg(selected->getSellValue()));sellBtn->setEnabled(true);
    } else {
        spriteLabel->clear();unitName->setText("未选中宝可梦");
        attackLabel->setText("点选地图上的我方宝可梦查看详情");attackLabel->setToolTip("");
        upgradePreview->setText("选择单位查看升级收益");upgradePreview->setToolTip("");
        for(int i=0;i<6;++i)unitValues[i]->setText("—");
        for(int i=0;i<4;++i)strategyTabs[i]->setEnabled(false);
        upgradeBtn->setText("升级\n—");sellBtn->setText("出售\n—");
        upgradeBtn->setEnabled(false);sellBtn->setEnabled(false);
    }

    const Statistics& st = game->getStatistics();
    const QString state=game->isOver() ? (game->isVictory() ? "胜利" : "失败") :
                        game->isPaused() ? "暂停" : game->isRunning() ? "进行中" : "待开始";
    const QString values[]={QString::number(game->getMap().getEnemyCount()),
        QString::number(game->getMap().getDefenderCount()),
        QString("%1/%2").arg(game->getMap().getCore().getHp()).arg(game->getMap().getCore().getMaxHp()),
        state,QString::number(st.getKills()),QString::number(st.getReactionsTriggered()),
        QString::number(st.getEnemiesLeaked()),QString::number(st.getGoldEarned())};
    for(int i=0;i<8;++i) statValues[i]->setText(values[i]);

    const std::vector<std::string>& logs = game->getLog();
    QStringList lines;
    int start = logs.size() > 8 ? static_cast<int>(logs.size()) - 8 : 0;
    for (int i = start; i < static_cast<int>(logs.size()); ++i) {
        lines << QString::fromStdString(logs[i]);
    }
    logLabel->setText(lines.isEmpty() ? QString("等待战斗开始\n选择宝可梦，在点位部署\n按“开始 / 下一波”迎敌\n击杀敌人可获得金币与升级机会") : lines.join("\n"));
}
