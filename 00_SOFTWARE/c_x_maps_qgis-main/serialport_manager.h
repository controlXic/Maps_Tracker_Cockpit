#ifndef SERIALPORT_MANAGER_H
#define SERIALPORT_MANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QDateTime>
#include <QDate>

struct NavState
{
    double lastLat = 0;
    double lastLon = 0;

    double vx = 0;
    double vy = 0;

    qint64 lastImuTime = 0;
    qint64 lastGpsTime = 0;
};

class serialport_manager : public QObject
{
    Q_OBJECT

public:
    explicit serialport_manager(QObject *parent = nullptr);

    void write_serial_data(QByteArray data);

    QDateTime accessible_dateTime();

    // i2c class functions //
    void update_gps_velocity(double lat, double lon);
    void save_velocity_log();

signals:
    void gps_updated(QString gpsData);

signals:
    void vel_status(double vG);

private slots:
    void init_serial_port();

    void process_serial_data(QByteArray gpsData);

    void on_ready_read();

    double nmea_to_decimal(const QString& value, const QString& hemisphere);

    QDate get_last_sunday(int year, int month);

    // i2c class functions //
    double deg2rad(double deg);
    double gps_distance(double lat1, double lon1, double lat2, double lon2);

private:
    QDateTime rtDataHora;

};

#endif // SERIALPORT_MANAGER_H
