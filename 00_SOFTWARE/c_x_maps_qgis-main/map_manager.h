#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include <QObject>
#include <QVector>
#include <QDateTime>

#include <qgsvectorlayer.h>
#include "serialport_manager.h"

struct GpsPoint
{
    double lat;
    double lon;
    double prevLat;
    double prevLon;
};

struct initRouteVars
{
    double initKm;
    QDateTime initDateHour;
};

class map_manager : public QObject
{
    Q_OBJECT

public:
    explicit map_manager(QObject *parent = nullptr);

    double calculate_track_distance(const QVector<GpsPoint>& points);

    void get_track_settings(QString gpxPath);

    void save_route_points();

    void load_route_points();

    void save_route_waypoints();

    void load_route_waypoints();

    initRouteVars route_init_status();

    double calculate_km_route(GpsPoint point);

    QString calculate_time_route(QDateTime lastTime);


private:
    GpsPoint prevPoint;

    // Declaració d'objectes //
    serialport_manager *spMangr = nullptr;

};

#endif // MAP_MANAGER_H
