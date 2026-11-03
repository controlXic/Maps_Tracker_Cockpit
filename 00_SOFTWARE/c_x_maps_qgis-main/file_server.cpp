//  // File server //
//  saber IP: ip a
//  iniciar server: python server.py
//  kill server: sudo killall python
//  open server: python -m http.server 5000 // python -m http.server 5000 --bind 0.0.0.0
//  close server: ctrl+c //// ps aux | grep http.server .:despres:.--> kill -9 nPID(primer num despres d'orangepi)
//  address server: http://192.168.1.22:5000
//
//  // WIFI //
//  restart lan/wpa: sudo systemctl restart NetworkManager
//  restart lan/wpa: sudo systemctl restart wpa_supplicant
//  veure procesos: ps aux | grep create_ap
//  acabar proces: kill -9 nPID
//  trobar xarxes: sudo iw dev wlan0 scan | grep SSID
//  connectar: sudo wpa_passphrase "MiFibra-1976" "benvinguts2018" > wifi.conf && sudo wpa_supplicant -B -i wlan0 -c wifi.conf && sudo dhclient wlan0
//
//  // Comandes wifi definitives per anular l'Acess Point//
//  sudo nano /etc/NetworkManager/NetworkManager.conf (comentar/borrar #unmanaged-devices:)
//  sudo systemctl restart NetworkManager

//



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








