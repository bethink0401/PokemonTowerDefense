QT += widgets

CONFIG += c++17
TEMPLATE = app
TARGET = PokemonTowerDefense

INCLUDEPATH += \
    core \
    entity \
    battle \
    strategy \
    event \
    persistence \
    template \
    ui

SOURCES += main.cpp \
    $$files(core/*.cpp, true) \
    $$files(entity/*.cpp, true) \
    $$files(battle/*.cpp, true) \
    $$files(strategy/*.cpp, true) \
    $$files(event/*.cpp, true) \
    $$files(persistence/*.cpp, true) \
    $$files(ui/*.cpp, true)

HEADERS += \
    $$files(core/*.h, true) \
    $$files(entity/*.h, true) \
    $$files(battle/*.h, true) \
    $$files(strategy/*.h, true) \
    $$files(event/*.h, true) \
    $$files(persistence/*.h, true) \
    $$files(template/*.h, true) \
    $$files(ui/*.h, true)
