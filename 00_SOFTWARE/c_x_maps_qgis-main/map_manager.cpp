#include "map_manager.h"

#include "mainwindow.h"
#include <QVector>
#include <cmath>

#include <qgsdistancearea.h>
#include <qgspointxy.h>



// track vars //
QVector<GpsPoint> track;

// route vars //
QString routeRtPath =    "../../../01_FILES/ROUTE/current_route.txt";
QString waypointRtPath = "../../../01_FILES/ROUTE/current_waypoint.txt";



map_manager::map_manager(QObject *parent)
    : QObject{parent}
{
    spMangr = new serialport_manager(this);
}



// Calcula km del track punt a punt //
double map_manager::calculate_track_distance(const QVector<GpsPoint>& points)
{
    if (points.size() < 2)
        return 0.0;

    QgsDistanceArea dist;
    dist.setEllipsoid("WGS84");

    double totalMeters = 0.0;
    trackPoint[3][0] = 0;

    for (int i = 1; i < points.size(); ++i)
    {
        totalMeters += dist.measureLine(
            QgsPointXY(points[i-1].lon, points[i-1].lat),
            QgsPointXY(points[i].lon,   points[i].lat)
        );
        // Assigna Km a track point //
        trackPoint[3][i] = totalMeters / 1000.0;
    }

    return totalMeters / 1000.0;
}

// Extreure coordenades del track //
void map_manager::get_track_settings(QString gpxPath)
{
    // Borrar Track anterior //
    for (int i = 0; i <= trackPointNum; ++i)
    {
        for (int j = 0; j <= 3; ++j)
            trackPoint[j][i] = 0;
    }
    trackPointNum = 0;
    maxElevation = 0;
    kmTotal = 0;
    track.clear();

    // Desar punts del track //
    QString uri = gpxPath + "|layername=" + "tracks";

    QgsVectorLayer *layer = new QgsVectorLayer(uri, "tracks", "ogr");

    if (!layer->isValid())
    {
        qDebug() << "Capa GPX invalida:";
        delete layer;
    }

    QgsFeatureIterator it = layer->getFeatures();
    QgsFeature feature;

    while (it.nextFeature(feature))
    {
        QgsGeometry geom = feature.geometry();
        if (geom.isNull())
            continue;

        QgsVertexIterator vIt = geom.vertices();
        while (vIt.hasNext())
        {
            QgsPoint pt = vIt.next();

            double lat = pt.y();
            double lon = pt.x();
            double ele = pt.z();

            // Guardar coordenades del track //
            trackPoint[0][trackPointNum] = lat;
            trackPoint[1][trackPointNum] = lon;
            trackPoint[2][trackPointNum] = ele;

            // Punts del track nº de coordenades //
            trackPointNum++;

            // Calcul elevacio maxima //
            if(pt.z() > maxElevation)
                maxElevation = pt.z();
        }
    }

    // Calcular km totals //
    for(int i = 0; i < trackPointNum; i++)
    {
        track.append({(trackPoint[0][i]), (trackPoint[1][i])});
    }
    kmTotal = calculate_track_distance(track);
}


// Gestio de la ruta activa al tancar i obrir l'App //
void map_manager::save_route_points()
{
    qDebug() << "save_route_points";

    if (routeDateHour.size() != routePointNum)
    {
        qDebug() << "Error: routeDateHour i routePointNum no son iguals";
        qDebug() << "routePointNum" << routePointNum;
        qDebug() << "routeDateHour.size()" << routeDateHour.size();
//        return;

        // Arreglo per si hi ha més punts de ruta que hores guardets //
        if (routeDateHour.size() < routePointNum)
        {
            int dif;
            dif = routePointNum - routeDateHour.size();

            for(int i = 0; i < dif; i++)
            {
                routePointNum = routePointNum - 1;
            }
            routePointCnt = routePointNum;

            qDebug() << "routeDateHour.size() < routePointNum" ;
            qDebug() << "routePointNum" << routePointNum;
            qDebug() << "routeDateHour.size()" << routeDateHour.size();
        }

        // routeDateHour.size() > routePointNum //
        else
        {
            // igualar contadors //
            int dif;
            dif = routeDateHour.size() - routePointNum;

            for(int i = 0; i < dif; i++)
                routeDateHour.removeAt(i);

            qDebug() << "routeDateHour.size() > routePointNum" ;
            qDebug() << "routePointNum" << routePointNum;
            qDebug() << "routeDateHour.size()" << routeDateHour.size();
        }

        return;
    }

    // Obrir arxiu //
    QFile file(routeRtPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "error route file, not open";
        file.close();
        return;
    }

    QTextStream out(&file);

    // validar coordenades, != 0  //
    uint16_t validCoord = 0;
    for (int i = 0; i < routePointNum; i++)
    {
        if(routePoint[0][i] != 0)
        {
            validCoord++;
        }
    }
    routePointNum = routePointCnt = validCoord;

    // Guardar numero de punts //
    out << routePointNum << "\n";

    // Guardar dades  //
    for (int i = 0; i < routePointNum; i++)
    {
        if(routePoint[0][i] != 0)
        {
            out << routePoint[0][i] << " "
                << routePoint[1][i] << " "
                << routePoint[2][i] << " "
                << routeDateHour[i] << "\n";
        }
    }

    file.close();
}


void map_manager::load_route_points()
{
    QFile file(routeRtPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Error al obrir arxiu load route";
        return;
    }

    QTextStream in(&file);

    // Neteja llista //
    routeDateHour.clear();

    // Llegir numero de puntos //
    in >> routePointNum;
    routePointCnt = routePointNum;

    if (routePointNum > 50000)
    {
        qDebug() << "over points in route";
        file.close();
        return;
    }

    // Leer datos
    for (int i = 0; i < routePointNum; i++)
    {
        QString fechaHora;

        in >> routePoint[0][i]
           >> routePoint[1][i]
           >> routePoint[2][i]
           >> fechaHora;

        routeDateHour.append(fechaHora);
    }

    file.close();
}


// Gestio de la ruta waypoint activa al tancar i obrir l'App //
void map_manager::save_route_waypoints()
{
    if (waypointDateHour.size() != waypointNum)
    {
        qDebug() << "Error: waypointDateHour i waypointPointNum no son iguals";
        qDebug() << "waypointNum" << waypointNum;
        qDebug() << "waypointDateHour.size()" << waypointDateHour.size();

        return;
    }

    QFile file(waypointRtPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "error waypoint file, not open";
        return;
    }

    QTextStream out(&file);

    // Guardar numero de punts //
    out << waypointNum << "\n";

    // Guardar datos
    for (int i = 0; i < waypointNum; i++)
    {
        if(waypoint[0][i] != 0)
        {
            out << waypoint[0][i] << " "
                << waypoint[1][i] << " "
                << waypoint[2][i] << " "
                << waypointDateHour[i] << "\n";
        }
    }

    file.close();
}


void map_manager::load_route_waypoints()
{
    QFile file(waypointRtPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Error al obrir arxiu load waypoint";
        return;
    }

    QTextStream in(&file);

    // Limpiar lista antes de cargar
    waypointDateHour.clear();

    // Llegir numero de punts //
    in >> waypointNum;
    waypointCnt = waypointNum;
//    qDebug() << "waypointNum" << waypointNum;


    if (waypointNum > 50000)
    {
        qDebug() << "over points in waypoint";
        file.close();
        return;
    }

    // Leer datos
    for (int i = 0; i < waypointNum; i++)
    {

        QString fechaHora;

        in >> waypoint[0][i]
           >> waypoint[1][i]
           >> waypoint[2][i]
           >> fechaHora;

        waypointDateHour.append(fechaHora);

//        // debug ruta //
//        qDebug() <<  waypoint[0][i];
//        qDebug() <<  waypoint[1][i];
//        qDebug() <<  waypoint[2][i];
//        qDebug() <<  fechaHora;
    }

    file.close();
}


initRouteVars map_manager::route_init_status()
{
    initRouteVars result;
    result.initKm = 0.0;
    result.initDateHour = QDateTime();

    double routePoint[4][50000];
    QVector<double> lat;
    QVector<double> lon;
    QVector<double> ele;
    QVector<QDateTime> timestamps;

    /////////////////
    // Obrir arxiu //
    QFile file(routeRtPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "route_init_status: Error al obrir routeFile";
        file.close();
        return result;
    }

    QTextStream in(&file);

    if(in.readLine() == "0")
    {
        qDebug() << "route_init_status: zero route points (EXIT)";
        file.close();
        return result;
    }

    in.readLine(); // saltar primera
    QString line = in.readLine();

    ///////////////////
    // Extreure hora //
    QString dataHoraStr = line.split(" ")[3];

    // Si no es UTC, treure la Z //
    dataHoraStr.chop(1);

    QDateTime inici = QDateTime::fromString(dataHoraStr, Qt::ISODate);

    /////////////////////////////
    // Extreure km recorreguts //
    in.seek(0);
    line = in.readLine();
    int nPoints = line.toInt();

    // Calcular distancia //
    for (int i = 0; i < nPoints; ++i) {
        line = in.readLine();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 4) continue;

        lat.append(parts[0].toDouble());
        lon.append(parts[1].toDouble());
        ele.append(parts[2].toDouble());
        timestamps.append(QDateTime::fromString(parts[3], Qt::ISODate));
    }

    // verifica numero de punt/contingut //
    if (lat.size() < 2)
    {
        qDebug() << "route_init_status: Calcular distancia (EXIT)" << "lat.size()" << lat.size();
        file.close();
        return result;
    }

    QgsDistanceArea dist;
    dist.setEllipsoid("WGS84");

    double totalMeters = 0.0;
    routePoint[3][0] = 0.0;

    for (int i = 1; i < lat.size(); ++i) {
        totalMeters += dist.measureLine(
            QgsPointXY(lon[i-1], lat[i-1]),
            QgsPointXY(lon[i], lat[i])
        );
        routePoint[3][i] = totalMeters / 1000.0;  // km acumulats
    }
    file.close();

    qDebug() << "Total Kms:" << totalMeters/ 1000.0 << "initRouteTime" << inici;

    result.initKm = totalMeters / 1000.0;
    result.initDateHour = inici;

    return result;
}

// suma km de la ruta amb nou punt/coord //
double map_manager::calculate_km_route(GpsPoint point)
{
    if (point.prevLat == 0)
        return 0.0;

    QgsDistanceArea dist;
    dist.setEllipsoid("WGS84");
    double totalMeters = 0.0;

    totalMeters += dist.measureLine(
        QgsPointXY(point.prevLon, point.prevLat),
        QgsPointXY(point.lon,   point.lat)
    );

    return totalMeters / 1000.0;
}

 QString map_manager::calculate_time_route(QDateTime lastTime)
 {
     QDateTime actual = spMangr->accessible_dateTime();

     // Calcular diferencia correctament //
     qint64 segonsTotals = lastTime.secsTo(actual); // segons reals

     qint64 hores = segonsTotals / 3600;
     qint64 minuts = (segonsTotals % 3600) / 60;
     qint64 segons = segonsTotals % 60;

//     qDebug() << "lastTime:" << lastTime.time().toString("HH:mm:ss");
//     qDebug() << "Ara:" << actual.time().toString("HH:mm:ss");
//     qDebug() << "Transcurregut:" << QString("%1:%2:%3")
//                                     .arg(hores,2,10,QChar('0'))
//                                     .arg(minuts,2,10,QChar('0'))
//                                     .arg(segons,2,10,QChar('0'));

     QString routeTime = QString("%1:%2")
             .arg(hores, 2, 10, QChar('0'))
             .arg(minuts, 2, 10, QChar('0'));

     return routeTime;

 }
