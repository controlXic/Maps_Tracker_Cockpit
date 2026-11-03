#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <QObject>
#include <QTimer>

struct NavState
{
    double lastLat = 0;
    double lastLon = 0;

    double vx = 0;
    double vy = 0;

    qint64 lastImuTime = 0;
    qint64 lastGpsTime = 0;
};

class i2c_manager : public QObject
{
    Q_OBJECT

public:
    explicit i2c_manager(QObject *parent = nullptr);

    bool init_i2c();

    void start_imu_timer(int msPeriode);

signals:

    void accel_status(double xAcc,
                   double yAcc,
                   double zAcc,
                   double xAng,
                   double yAng,
                   double zAng);

private slots:
    void update_angle();

    void update_acceleration();

private:
    int read_i2c_word(int reg);

    // Vars //
    int fileDes;
    int16_t xAng, yAng, zAng;
    int16_t accX, accY, accZ;
    QTimer i2cReadPeriode;

};

#endif // I2C_MANAGER_H
