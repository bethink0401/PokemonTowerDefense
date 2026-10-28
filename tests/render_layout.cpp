#include "MainWindow.h"
#include "GameWidget.h"
#include "PokemonPanel.h"
#include "InfoPanel.h"
#include <QApplication>
#include <iostream>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QEventLoop>
#include "TargetStrategy.h"
#include <cassert>
#include "AssetManager.h"
#include <QFontDatabase>
#include "Game.h"
#include "UnitInsights.h"
#include "Pokemon.h"
#include "ForestMapData.h"
#include "Enemy.h"
#include "StatusEffect.h"
#include "ElementStatus.h"
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");
    Game* sample=nullptr;
    GameWidget* battleView=nullptr;
    MainWindow w;
    for (auto* child : w.findChildren<QWidget*>()) {
        auto* view = dynamic_cast<GameWidget*>(child);
        if (!view) continue;
        sample=view->getGame();battleView=view;
        int placed=0;
        for (int y=0; y<Position::MAP_HEIGHT && placed<3; ++y)
            for (int x=0; x<Position::MAP_WIDTH && placed<3; ++x)
                if (sample->isDeployable(Position(x,y))) {
                    auto* pokemon = sample->deployPokemon("Pichu",Position(x,y));
                    if (pokemon) { ++placed; view->setSelected(pokemon); }
                }
        sample->getMap().addEnemy(new BasicEnemy());
        auto* tank = new TankEnemy();
        sample->getMap().addEnemy(tank);
        tank->move(16);
        for(int i=0;i<100;++i) tank->addStatusEffect(new SlowEffect(2.5,.45));
        sample->getReactionEngine().applyElement(*tank,PokemonType::Fire,1,8);
        sample->getReactionEngine().applyElement(*tank,PokemonType::Water,1,8);
    }
    sample->getPlayer().addGold(500);
    assert(sample->deployPokemon("Squirtle",Position(35,9)));
    auto route=ForestMap::pathCave();
    for(size_t i=0;i+1<route.size();++i) {
        if(std::hypot(route[i].x-32.5,route[i].y-7.0)<.6) {
            sample->getMap().getEnemies().back()->setPath({route[i],route[i+1]});break;
        }
    }
    battleView->resetBattleVisuals();
    w.resize(1380,760);
    w.show();
    app.processEvents();
    QEventLoop frame;QTimer::singleShot(40,&frame,&QEventLoop::quit);frame.exec();
    auto strategies=w.findChildren<QPushButton*>("strategyTab");
    assert(strategies.size()==4);
    for(auto* tab : strategies) {
        assert(tab->isVisible() && tab->isEnabled());
        if(tab->property("strategyId").toString()=="Fastest") tab->click();
    }
    for(int i=0;i<3;++i) {
        auto* level=w.findChild<QLabel*>(QString("evolutionLevel%1").arg(i));
        assert(level && level->text()==QString("Lv.%1").arg(i==0?1:i==1?3:5));
    }
    for(int i=0;i<6;++i) {
        auto* detail=w.findChild<QLabel*>(QString("unitValue%1").arg(i));
        assert(detail && detail->isVisible() && detail->text()!="—");
    }
    bool changed=false;
    for(Pokemon* pokemon : sample->getMap().getDefenders())
        if(pokemon->getStrategy() && pokemon->getStrategy()->getName()=="Fastest") changed=true;
    assert(changed);
    assert(w.findChild<QLabel*>("unitValue3")->text()=="就绪");
    assert(w.findChild<QLabel*>("unitValue4")->text()=="1");
    assert(!w.findChild<QLabel*>("unitSynergy"));
    battleView->getSelected()->attack(*sample->getMap().getEnemies().back());
    battleView->setSelected(battleView->getSelected());
    assert(w.findChild<QLabel*>("unitValue3")->text().endsWith(" 秒"));
    assert(w.findChild<QLabel*>("upgradePreview")->text().contains("攻击提高"));
    assert(!w.findChild<QLabel*>("upgradePreview")->text().contains("→"));
    for(auto* label:w.findChildren<QLabel*>())assert(label->text()!="进化链");
    auto* firstLevel=w.findChild<QLabel*>("evolutionLevel0");
    for(auto* button:w.findChildren<QPushButton*>()) {
        if(button->text().startsWith("升级\n")||button->text().startsWith("出售\n"))
            assert(button->mapTo(&w,QPoint(0,button->height())).y()<=firstLevel->mapTo(&w,QPoint()).y());
        if(button->objectName()=="entryLight")assert(button->font().bold());
    }
    for(const auto* name : {"start","pause","restart","save","load","upgrade","sell"})
        assert(!AssetManager::instance().actionIcon(name).isNull());
    for(const auto* name : {"fire","water","grass","electric","slow","burn","paralyze"})
        assert(!AssetManager::instance().statusIcon(name).isNull());
    for(const auto* name : {"strategy_nearest","strategy_lowesthp","strategy_closesttocore","strategy_fastest","menu_down"})
        assert(!AssetManager::instance().actionIcon(name).isNull());
    for(int i=0;i<8;++i) {
        auto* value=w.findChild<QLabel*>(QString("statValue%1").arg(i));
        assert(value && value->isVisible());
        if(i%4) {
            auto* previous=w.findChild<QLabel*>(QString("statValue%1").arg(i-1));
            assert(value->parentWidget()->x()>previous->parentWidget()->x());
            assert(std::abs(value->parentWidget()->width()-previous->parentWidget()->width())<=1);
        }
    }
    if (!w.grab().save("assets/maps/_layout_preview.png")) return 1;
    for (auto size : {QSize(1380,760), QSize(1100,680)}) {
        w.resize(size);
        app.processEvents();
        auto* detail=w.findChild<QLabel*>("unitValue0");
        assert(detail->fontMetrics().horizontalAdvance(detail->text())<=detail->contentsRect().width());
        assert(w.findChild<QLabel*>("unitValue3")->fontMetrics().horizontalAdvance("就绪")<=w.findChild<QLabel*>("unitValue3")->contentsRect().width());
        if(size==QSize(1100,680))assert(w.grab().save("assets/maps/_layout_small.png"));
        if(w.size()!=size) { std::cerr << "Requested window size did not fit\n"; return 3; }
        for (auto* child : w.centralWidget()->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)) {
            if (!w.centralWidget()->rect().contains(child->geometry())) {
                std::cerr << "Layout overflow\n"; return 2;
            }
        }
    }
    sample->getPlayer().addGold(1000);
    Pokemon* selected=battleView->getSelected();
    assert(selected && sample->upgradePokemon(selected));
    battleView->setSelected(selected);
    assert(w.findChild<QLabel*>("unitName")->text().contains("Lv.2"));
    assert(w.findChild<QLabel*>("unitValue0")->text()==QString::number(UnitInsights::dps(*selected),'f',1));
    bool priceUpdated=false;
    for(auto* button : w.findChildren<QPushButton*>()) if(button->text()=="进化\n160 金币")priceUpdated=true;
    assert(priceUpdated);
    assert(sample->upgradePokemon(selected));battleView->setSelected(selected);
    assert(w.findChild<QLabel*>("unitName")->text().contains("皮卡丘 Lv.3"));
    assert(w.findChild<QLabel*>("evolutionLevel1")->text()=="Lv.3");
    assert(w.findChild<QLabel*>("unitAttack")->text().contains("一阶进化"));
    battleView->setSelected(nullptr);
    for(auto* tab : strategies) assert(!tab->isEnabled());
    for(int i=0;i<6;++i) assert(w.findChild<QLabel*>(QString("unitValue%1").arg(i))->text()=="—");
    std::cout << "PASS: selection/upgrade/evolution detail refresh,  persistent strategy tabs, per-form levels, six live unit metrics; layout fits 1380x760 and 1100x680\n";
}


