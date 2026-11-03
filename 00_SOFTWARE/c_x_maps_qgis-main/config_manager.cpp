#include "config_manager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QDebug>



QString path = "../../../01_FILES/CONFIG/c_x_maps_config.json";


// Track Tab //
QString trackName;
bool trackActive;
bool routeActive;

// View Tab //
bool placesActive;
bool fuelsActive;
bool trackInfoActive;
bool nightModeActive;
bool fullScreenActive;

// Cockpit Tab //
QString cntKms;

config_manager::config_manager()
{
    set_config_json();
}



void config_manager::set_config_json()
{
    // Crear object JSON //
    QJsonObject jObject;

    // Track Tab //
    jObject["track_active"] = trackActive;
    jObject["track_name"] = trackName;
    jObject["route_active"] = routeActive;

    // View Tab //
    jObject["places_active"] = placesActive;
    jObject["fuels_active"] = fuelsActive;
    jObject["trackInfo_active"] = trackInfoActive;
    jObject["nightMode_active"] = nightModeActive;
    jObject["fullScreen_active"] = fullScreenActive;

    // Cockpit Tab //
    jObject["odometer"] = cntKms;

    // cockpit conta kms //

    // Convertir a document  JSON //
    QJsonDocument doc(jObject);

    QFile jFile(path);
    if (jFile.open(QIODevice::WriteOnly))
    {
        jFile.write(doc.toJson(QJsonDocument::Indented));
        jFile.close();
//        qDebug() << "jFile JSON make success";
    }
    else
    {
        qDebug() << "Error make jFile";
    }
}

void config_manager::write_config_json(QString objName, bool objStatus, QString fName)
{
    QFile jFile(path);
    if (!jFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error open .json write file";
        return;
    }

    QByteArray jData = jFile.readAll();
    jFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(jData);
    QJsonObject jObject = doc.object();

    // Modificar json data //
    if(fName != "")
        jObject[objName] = fName;
    else
        jObject[objName] = objStatus;

    // Guardar de nou //
    QJsonDocument newDoc(jObject);
    if (jFile.open(QIODevice::WriteOnly))
    {
        jFile.write(newDoc.toJson(QJsonDocument::Indented));
        jFile.close();
//        qDebug() << "JSON edit & save sucess";
    }
    else
    {
        qDebug() << "Error save .json file";
    }
}


void config_manager::read_config_json()
{
    QFile jFile(path);

    if (!jFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error: open .json file";
        return;
    }

    QByteArray datos = jFile.readAll();
    jFile.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(datos, &error);

    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "Error parser JSON:" << error.errorString();
        return;
    }

    if (!doc.isObject())
    {
        qDebug() << "JSON object error.";
        return;
    }

    QJsonObject jObject = doc.object();

    // Assignar valors a variables //
    // Track Tab //
    trackActive = jObject["track_active"].toBool();
    trackName = jObject["track_name"].toString();
    routeActive = jObject["route_active"].toBool();

    // View Tab //
    placesActive = jObject["places_active"].toBool();
    fuelsActive = jObject["fuels_active"].toBool();
    trackInfoActive = jObject["trackInfo_active"].toBool();
    nightModeActive = jObject["nightMode_active"].toBool();
    fullScreenActive = jObject["fullScreen_active"].toBool();

    // Cockpit Tab //
    cntKms = jObject["odometer"].toString();
}

