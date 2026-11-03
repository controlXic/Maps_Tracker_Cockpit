#include "i2c_manager.h"

#include <wiringPi.h>
#include <wiringPiI2C.h>

#include <QDateTime>
#include <QDebug>
#include <QtMath>
#include <QFile>


#define MPU6050_ADDR 0x68


i2c_manager::i2c_manager(QObject *parent)
    : QObject{parent}
{
    //connect(&i2cReadPeriode, &QTimer::timeout, this, &i2c_manager::update_angle);
    //init_i2c();
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool i2c_manager::init_i2c()
{
    wiringPiSetup();

    fileDes = wiringPiI2CSetupInterface("/dev/i2c-2", MPU6050_ADDR);

    if(fileDes < 0)
    {
        qDebug() << "I2C open error";
        return false;
    }

    int id = wiringPiI2CReadReg8(fileDes, 0x75);

    if(id != 0x68)
    {
        qDebug() << "MPU6050 not detected";
        return false;
    }

    wiringPiI2CWriteReg8(fileDes, 0x6B, 0); // wake sensor

    qDebug() << "MPU6050 initialized" << " - fileDes:" << fileDes;

    return true;
}

void i2c_manager::start_imu_timer(int msPeriode)
{
    i2cReadPeriode.start(msPeriode);
}

int i2c_manager::read_i2c_word(int reg)
{
    int val = wiringPiI2CReadReg16(fileDes, reg);

    val = ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);

    return (int16_t)val;
}

// Llegir angles //
void i2c_manager::update_angle()
{
    uint16_t minVal = 265;
    uint16_t maxVal = 402;

    // Lectura de sensors //
    int16_t rawGx = read_i2c_word(0x3B);
    int16_t rawGy = read_i2c_word(0x3D);
    int16_t rawGz = read_i2c_word(0x3F);

    rawGx = map(rawGx, minVal, maxVal, -90, 90);
    rawGy = map(rawGy, minVal, maxVal, -90, 90);
    rawGz = map(rawGz, minVal, maxVal, -90, 90);

    xAng = qRadiansToDegrees(qAtan2(-rawGy, -rawGz)+ M_PI);
    yAng = qRadiansToDegrees(qAtan2(-rawGx, -rawGz)+ M_PI);
    zAng = qRadiansToDegrees(qAtan2(-rawGy, -rawGx)+ M_PI);

//    qDebug() << "xAng" << xAng << "yAng" << yAng << "zAng" << zAng;
//    qDebug() << "rawGx" << rawGx << "rawGy" << rawGy << "rawGz" << rawGz;

    update_acceleration();
}

// Llegir acceleracio //
void i2c_manager::update_acceleration()
{
    // Leer acelerómetro
      int16_t rawAx = read_i2c_word(0x3B);
      int16_t rawAy = read_i2c_word(0x3D);
      int16_t rawAz = read_i2c_word(0x3F);

      // Convertir a g
      float ax = rawAx / 16384.0;
      float ay = rawAy / 16384.0;
      float az = rawAz / 16384.0;

      // Convertir ángulos a radianes
      float pitch = qDegreesToRadians((double)xAng);
      float roll  = qDegreesToRadians((double)yAng);

      // Vector gravedad estimado
      float gx = sin(pitch);
      float gy = -sin(roll);
      float gz = cos(pitch) * cos(roll);

      // Quitar gravedad → aceleración lineal
      float ax_lin = ax - gx;
      float ay_lin = ay - gy;
      float az_lin = az - gz;

      // Convertir a m/s²
      accX = ax_lin * 9.81;
      accY = ay_lin * 9.81;
      accZ = az_lin * 9.81;

  //    qDebug() << "AccX:" << accX << "AccY:" << accY << "AccZ:" << accZ;


    emit accel_status(accX, accY, accZ, xAng, yAng, zAng);
}
