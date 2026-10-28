// =============================================================
//  main.cpp
//  程序入口：创建 Qt 应用与主窗口
// =============================================================
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PokemonTowerDefense");

    MainWindow window;
    window.resize(1380, 760);
    window.show();

    return app.exec();
}
