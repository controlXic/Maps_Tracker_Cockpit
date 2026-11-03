#include "file_server.h"

#include <QProcess>
#include <QDebug>
#include <QTimer>

QProcess *createWifiAP = new QProcess();
QProcess *startServer = new QProcess();
QProcess *killWifi = new QProcess();



file_server::file_server(QObject *parent)
    : QObject{parent}
{

}

void file_server::open_wifi()
{
    createWifiAP->start("bash", QStringList() << "-c" << "sudo create_ap -m nat wlan0 end0 cXic-Husqvarna montenegro --no-virt");

//    qDebug() << "wifi start";
//    qDebug() << createWifiAP->processId();
}

void file_server::close_wifi()
{
    createWifiAP->terminate();
    createWifiAP->waitForFinished(3000);

//    qDebug() <<  "wifi Kill";
}

void file_server::start_file_server()
{
    startServer->setWorkingDirectory("../../../01_FILES/GPX_SERVER");
    startServer->start("bash", QStringList() << "-c" << "python server.py");

//    qDebug() <<  "Server started";
}

void file_server::stop_file_server()
{
    startServer->terminate();
    startServer->waitForFinished(3000);

//    qDebug() <<  "Server Kill";
}








