#include "serialport_manager.h"

#include "mainwindow.h"

#include <QSerialPort>
#include <QDebug>
#include <QStringList>
#include <qgspointxy.h>


// Permisos SerialPort CMD //
// sudo usermod -a -G dialout $USER
// groups
// ls /dev/ttyS*: /dev/ttyS0  /dev/ttyS1  /dev/ttyS2  /dev/ttyS3  /dev/ttyS4  /dev/ttyS5
// test CMD: gpio serial /dev/ttyS5

// GPS msg //
// $GPGGA,115543.00,4120.54597,N,00143.28518,E,1,06,1.48,213.9,M,49.5,M,,*5C


QSerialPort *serial = new QSerialPort();
QByteArray serialArr;
QStringList serialStrList;
uint8_t strCnt = 0;
QString serialStr;

QString velLogPath =  "../../../01_FILES/ROUTE/velocity_log.txt";
double velocitats[5];

serialport_manager::serialport_manager(QObject *parent)
    : QObject{parent}
{
    init_serial_port();

    connect(serial, &QSerialPort::readyRead, this, &serialport_manager::on_ready_read);
}

void serialport_manager::init_serial_port()
{
    serial->setPortName("/dev/ttyS5");
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->open(QIODevice::ReadWrite);
}

// En desus //
void serialport_manager::write_serial_data(QByteArray data)
{
    serial->write("\n" + data);
}

void serialport_manager::on_ready_read()
{
    serialArr.append(serial->readAll());

    while (true)
    {
        // Buscar inici del missatge //
        int start = serialArr.indexOf('$');
        if (start == -1) {
            serialArr.clear();
            return;
        }

        // Eliminar brossa avans de $ //
        if (start > 0)
            serialArr.remove(0, start);

        // Buscar següent '$' (inicio del següent missatge) //)
        int nextStart = serialArr.indexOf('$', 1);

        // Buscar final amb '\r' //
        int end = serialArr.indexOf('\r');
        int messageEnd = -1;

        // Cas: $...$  → talla abans del següent $ //
        if (nextStart != -1 && (end == -1 || nextStart < end))
            messageEnd = nextStart;

        // Cas: $...\r //
        else if (end != -1)
            messageEnd = end;

        // Missatge incomplert → esperar més dades //
        else
            return;

        QByteArray message = serialArr.left(messageEnd);

        // Eliminar del buffer //
        serialArr.remove(0, messageEnd);

        if (!message.startsWith('$'))
            continue;

        process_serial_data(message);
    }
}

void serialport_manager::process_serial_data(QByteArray gpsData)
{
    // Tab Test debug //
    emit gps_updated(gpsData);

    ////////////////////////////////////////
    // Extreu dades GPS del string serial //
    serialStr = gpsData;
    QStringList fields = serialStr.split(",");

    // Coordenades GPS //
    if (fields.size() >= 6 && fields[0].contains("GGA"))//GGA
    {
        QString latValue = fields[2];
        QString latHem   = fields[3];
        QString lonValue = fields[4];
        QString lonHem   = fields[5];

        double lat = nmea_to_decimal(latValue, latHem);
        double lon = nmea_to_decimal(lonValue, lonHem);
        double ele = fields[9].toDouble();


        if(lat > 0 && lon > 0 )
        {
            gpsPoint[0] = lat;
            gpsPoint[1] = lon;
            gpsPoint[2] = ele;
            gpsReadFlag = true;

//            qDebug() << "Rebut Lat, Lon, Ele: " << lat << lon << ele;
        }
    }

    // Data hora GPS //
    else if (fields.size() >= 10 && fields[0].contains("RMC"))
    {
        // treure  milisegons //
        QString timeStr = fields[1].section('.', 0, 0);

        QTime rtc = QTime::fromString(timeStr, "hhmmss");
        QDate rtd = QDate::fromString(fields[9], "ddMMyy");

        // Ajustar de any //
        if (rtd.isValid() && rtd.year() < 2000)
            rtd = rtd.addYears(100);

        if (rtc.isValid() && rtd.isValid())
        {
            QDate summerDay = get_last_sunday(rtd.year(), 3);
            QDate winterDay = get_last_sunday(rtd.year(), 10);

            if (rtd > summerDay && rtd < winterDay)
                 rtc = rtc.addSecs(7200);
            else
                 rtc = rtc.addSecs(3600);

            rtDataHora.setDate(rtd);
            rtDataHora.setTime(rtc);

//            qDebug() << "dataTime:" << rtDataHora;
        }
    }
}

QDate serialport_manager::get_last_sunday(int year, int month)
{
    QDate d(year, month, QDate(year, month, 1).daysInMonth());

    while (d.dayOfWeek() != Qt::Sunday)
        d = d.addDays(-1);

    return d;
}

// retorna data hora si segons la font habilitada //
QDateTime serialport_manager::accessible_dateTime()
{
    // Hora GPS //
    if(rtDataHora.isValid())
        return rtDataHora;
    // Hora Sistema //
    else
        return QDateTime::currentDateTime();
}

// convertir coordenades NEMA a ESPG:4326 //
double serialport_manager::nmea_to_decimal(const QString& value, const QString& hemisphere)
{
    if (value.isEmpty())
        return 0.0;

    double raw = value.toDouble();

    int degrees = 0;
    double minutes = 0.0;

    // Latitud te 2 graus, longitud 3 //
    // longitud (dddmm.mmmm) //
    if (value.length() > 7)
        degrees = int(raw / 100);
    // latitud (ddmm.mmmm) //
    else
        degrees = int(raw / 100);

    minutes = raw - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);

    // Sud y Oest son negatius //
    if (hemisphere == "S" || hemisphere == "W")
        decimal *= -1.0;

    return decimal;
}


double serialport_manager::deg2rad(double deg)
{
    return deg * M_PI / 180.0;
}

double serialport_manager::gps_distance(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000.0;

    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);

    lat1 = deg2rad(lat1);
    lat2 = deg2rad(lat2);

    double a = sin(dLat/2)*sin(dLat/2) +
               sin(dLon/2)*sin(dLon/2) *
               cos(lat1)*cos(lat2);

    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return R * c;
}

// velocitat gps //
void serialport_manager::update_gps_velocity(double lat, double lon)
{
    static NavState nav;

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if(nav.lastGpsTime == 0)
    {
        nav.lastLat = lat;
        nav.lastLon = lon;
        nav.lastGpsTime = now;
        return;
    }

    double dt = (now - nav.lastGpsTime) / 1000.0; //1000.0
    nav.lastGpsTime = now;

    // Velocitat GPS //
    double dist = gps_distance(nav.lastLat, nav.lastLon, lat, lon);

    double v_gps = dist / dt;

    nav.lastLat = lat;
    nav.lastLon = lon;

    // Velocitat IMU //
    double v_imu = sqrt(nav.vx*nav.vx + nav.vy*nav.vy);

    // ZUPT
    if(v_gps < 0.5)
    {
        nav.vx = 0;
        nav.vy = 0;
        v_imu = 0;
    }

    // Fusió //
    double alpha = 0.85;

    double v_final = alpha * v_gps + (1-alpha) * v_imu;

//    qDebug() << "GPS speed:" << v_gps*3.6;
//    qDebug() << "IMU speed:" << v_imu*3.6;
//    qDebug() << "Fusion speed:" << v_final*3.6;

    // copiar a buffer log //
    velocitats[0] = v_gps*3.6;
    velocitats[1] = v_imu*3.6;
    velocitats[2] = v_final*3.6;
    velocitats[3] = nav.vx;
    velocitats[4] = nav.vy;


    // Emetre valors //
    emit vel_status(v_gps*3.6);

}

void serialport_manager::save_velocity_log()
{
    // Obrir arxiu //
    QFile file(velLogPath);

    if (!file.open(QIODevice::Append | QIODevice::Text))
    {
        qDebug() << "error vel log file, not open";
        file.close();
        return;
    }

    QTextStream out(&file);

    // Guardar dades  //
    out << "v_gps: " << velocitats[0] << " --- "
        << "v_imu: " << velocitats[1] << " --- "
        << "v_final: " << velocitats[2] << " --- "
        << "nav.vx: " << velocitats[3] << " --- "
        << "nav.vy: " << velocitats[4] << "\n";

    file.close();
}
