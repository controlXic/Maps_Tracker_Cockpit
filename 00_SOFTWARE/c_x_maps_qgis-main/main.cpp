#include <QApplication>
#include "mainwindow.h"

#include <QCursor>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Init //
    QApplication a(argc, argv);
    a.setOverrideCursor(Qt::BlankCursor);

    MainWindow w;
    w.show();
//     w.showFullScreen();
     return a.exec();


 }

