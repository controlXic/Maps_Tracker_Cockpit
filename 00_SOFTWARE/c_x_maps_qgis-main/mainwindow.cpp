#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QLabel>
#include <QVector>
#include <QtCharts>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include <QTextStream>

#include <qgsmapcanvas.h>
#include <qgsproject.h>
#include <qgsrasterlayer.h>
#include <qgsvectortilelayer.h>
#include <qgsvectorlayer.h>
#include <qgsmapsettings.h>
#include <qgsvertexmarker.h>
#include <qgsmaptoolpan.h>
#include <qgsvectorfilewriter.h>
#include <qgsfield.h>

#include "switch.h"
#include "serialport_manager.h"
#include "config_manager.h"
//#include "i2c_manager.h"
#include "file_server.h"
#include "map_manager.h"
#include "gpio_manager.h"


// Vector Layer; capa de mapa //
QgsVectorTileLayer *vtLayer = new QgsVectorTileLayer();

// app update vars //
QTimer *updateCallbackTimer = new QTimer();
uint16_t saveRoutePeriode = 0;

// Layer ID vars //
QString tracksLayerId[10];
QString routesLayerId[10];
QString waypointsLayerId[10];
uint8_t trackCnt = 0;
QString placesLayerId = "";
QString fuelsLayerId = "";

// GPS vars //
float gpsPoint[3]; //0=Lat; 1=Lon; 2=Ele;
bool gpsReadFlag = false;

// Llista de gpx files //
QStringList gpxFiles;

// Vars per guardar dades del track/gpx //
float trackPoint[4][50000]; //0=Lat; 1=Lon; 2=Ele; 3=Km
uint16_t trackPointNum = 0;
uint32_t trackPointCnt = 0;
float maxElevation = 0;
float kmTotal = 0;
int trackListViewIndex = 0;

// Interpretacio del track/gpx //
double lastAngle = 0.0;
double routeAngle = 0;//90
uint8_t rotateMinAngle = 10; //10

// Simulacio track //
QTimer *gpsPosTimer = new QTimer();

// Vars per guardar dades de la ruta actual //
float routePoint[3][50000]; // 0=Lat; 1=Lon; 2=Ele;
uint16_t routePointCnt = 0;
uint16_t routePointNum = 0;
QStringList routeDateHour;
QString routeLayerRtId;
QgsFeatureId routeFid = -1;
QVector<QgsPoint> routePoints;
double kmsRouteNow = 0;
//QDateTime routeTime;

// Vars per guardar waypoints de la ruta actual //
float waypoint[3][500]; // 0=Lat; 1=Lon; 2=Ele;
uint16_t waypointCnt = 0;
uint16_t waypointNum = 0;
QStringList waypointDateHour;
QString waypointLayerRtId;

// vars de navegacio pel mapa //
//bool movManual = false;
bool centerAuto = true;

// View vars //
bool nightMode = false;

// COCKPIT vars //
double cntKmsCokpit = 0;
double initRouteKms = 0;
double initCockpitKms = 0;

// PowerOFF vars //
#define PWR_OFF_TIME 5 // En minuts
QTimer *pwrOffTimer = new QTimer();
QProcess *sysPwrOffPrcss = new QProcess();
bool swkl30Flg = true;
bool pwrOffFlg = false;

// Data paths //
QString mapPath =           "../../../01_FILES/MAPS/spain.mbtiles";
QString placesPath =        "../../../01_FILES/MAPS/spainPlaces.gpkg";
QString fuelsPath =         "../../../01_FILES/MAPS/spainFuels.gpkg";
QString gpxDirPath =        "../../../01_FILES/GPX_SERVER/GPX/";

QString mapStyle =          "../../../01_FILES/STYLES/styleMap.qml";
QString placesStyle =       "../../../01_FILES/STYLES/stylePlaces.qml";
QString fuelsStyle =        "../../../01_FILES/STYLES/styleFuels.qml";
QString gpxTracksStyle =    "../../../01_FILES/STYLES/styleTracksGpx.qml";
QString gpxRoutesStyle =    "../../../01_FILES/STYLES/styleRoutesGpx.qml";
QString gpxWaypointsStyle = "../../../01_FILES/STYLES/styleWaypointsGpx.qml";




/////////////////////////////////////////////////////////////////////////////////
// Init Window
/////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chartG(nullptr)
    , m_line(nullptr)
    , m_text(nullptr)
{
    ui->setupUi(this);

    // Treure icones de finestra //
    this->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);

    // Config map canvas //
    mCanvas = ui->mapWidget;
    mCanvas->setAttribute(Qt::WA_OpaquePaintEvent, true);
    mCanvas->setAttribute(Qt::WA_NoSystemBackground, true);
    mCanvas->setCanvasColor(QColor("#FBFBFB"));
    mCanvas->enableAntiAliasing(true);
    mCanvas->setParallelRenderingEnabled(true);
    mCanvas->setCachingEnabled(true);
    mCanvas->setMapUpdateInterval(250);//250=org pro=30-60
    mCanvas->setRenderFlag(true);
    mCanvas->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    // Config track canvas //
    trackCanvas = ui->trackWidget;
    trackCanvas->setCanvasColor(QColor("#FBFBFB"));
    trackCanvas->enableAntiAliasing(true);
    trackCanvas->setParallelRenderingEnabled(true);
    trackCanvas->setCachingEnabled(true);
    trackCanvas->setMapUpdateInterval(250);

    // registrar event mouse a eventFilter //
    mCanvas->viewport()->installEventFilter(this);

    // Icona de posicio //
    mMarker = new QgsVertexMarker(mCanvas);
    trackMarker = new QgsVertexMarker(trackCanvas);

    // Control moviment mapa amb mouse drag //
    panTool = new QgsMapToolPan(mCanvas);
    mCanvas->setMapTool(panTool);

    // inicialitzar objectes al constructor //
    spMangr = new serialport_manager(this);
//    i2cMangr = new i2c_manager(this);
    mapMangr = new map_manager(this);
    gpioMangr = new gpio_manager(this);

    // Simulacio GPS Timer //
    connect(gpsPosTimer, SIGNAL(timeout()), this, SLOT(simulate_route_points()));

    // Detecta canvi d'escala //
    connect(mCanvas, &QgsMapCanvas::scaleChanged, this, [](double factor)
    {
        qDebug() << "Wheel zoom factor:" << factor;
    });

    // App update callback Timer //
    connect(updateCallbackTimer, SIGNAL(timeout()), this, SLOT(app_update_callback()));
    updateCallbackTimer->start(100);

    connect(pwrOffTimer, SIGNAL(timeout()), this, SLOT(system_pwr_off()));


    // GPS callback //
    connect(spMangr, &serialport_manager::gps_updated, this, [this](QString gpsData)
    {
        ui->gpsTextEdit->append(gpsData);
    });

    // callback velocitat accelerometre //
    connect(spMangr, &serialport_manager::vel_status, this, [this](float vG)
    {
        // actualitzar cockpit //
        ui->velLavel->setText(QString::number(vG, 'f', 0) + " ");
        ui->accelGauge->setValue(vG);
    });
    //i2cMangr->start_imu_timer(300);

    // callback angle accelerometre //
//    connect(i2cMangr, &i2c_manager::accel_status, this, [this](double ax, double ay, double az, double xAng, double yAng, double zAng)
//    {
//        // actualitzar cockpit //
//        ui->giroFRLabel->setText(QString::number(yAng) + "º");
//        ui->giroLRLabel->setText(QString::number(zAng) + "º");
//    });

    // Detecta canvi d'estat classe switch custom //
    // (switch) city names on map//
    connect(ui->sPlaces, &QCheckBox::toggled, this, &MainWindow::toggle_view_places);

    // (switch) fuels on map //
    connect(ui->sFuels, &QCheckBox::toggled, this, &MainWindow::toggle_view_fuels);

    // (switch) label info on map //
    connect(ui->sInfo, &QCheckBox::toggled, this, &MainWindow::toggle_view_infoLabel);
    ui->infoLabel->setVisible(false);

    // (switch) night mode //
    connect(ui->sNight, &QCheckBox::toggled, this, &MainWindow::toggle_view_nightMode);

    // (switch) WIFI & File server //
    connect(ui->sWifi, &QCheckBox::toggled, this, &MainWindow::toggle_view_wifi);

    // (switch) full screen //
    connect(ui->sFullScrn, &QCheckBox::toggled, this, &MainWindow::toggle_view_fullScrn);

    // (switch) KL30 //
    connect(ui->sKL30, &QCheckBox::toggled, this, &MainWindow::toggle_view_kl30);

    // Funcions d'Inicialització //
    ui->tabWidget->setCurrentIndex(0);

    load_vector_map();
    set_route_layer();
    update_app_status();

    gpioMangr->gpio_init();
}

MainWindow::~MainWindow()
{
    qDebug() << "Quit App!!!!!";
    if(routePointNum > 0)
    {
        mapMangr->save_route_points();
        mapMangr->save_route_waypoints();
    }
    fServer->stop_file_server();
    fServer->close_wifi();

    delete mCanvas;
    delete trackCanvas;
    delete mMarker;
    delete trackMarker;
    mMarker = nullptr;
    trackMarker = nullptr;
    delete ui;
}


/////////////////////////////////////////////////////////////////////////////////
// App status
/////////////////////////////////////////////////////////////////////////////////

void MainWindow::app_update_callback()
{
    // Habilita lectura GPS un cop rep dades valides //
    if(gpsReadFlag)
    {
        // copiem track a ruta //
        routePoint[0][routePointCnt] = gpsPoint[0]; // Lat
        routePoint[1][routePointCnt] = gpsPoint[1]; // Lon
        routePoint[2][routePointCnt] = gpsPoint[2]; // Ele

        // pintem canvas //
        add_point_route(routePoint[0][routePointCnt], routePoint[1][routePointCnt], routePoint[2][routePointCnt]);        
        routeDateHour << spMangr->accessible_dateTime().toString("yyyy-MM-ddTHH:mm:ssZ");

        // centrem gps i cursor //
        if(routePointCnt > 1)
            routeAngle = map_rotation(routePoint[0][routePointCnt - 1], routePoint[1][routePointCnt - 1], routePoint[0][routePointCnt], routePoint[1][routePointCnt]);

        // Actualitzem canvas //
        set_gps_position(routePoint[0][routePointCnt], routePoint[1][routePointCnt]);

        // calcul de velocitat //
        spMangr->update_gps_velocity(routePoint[0][routePointCnt], routePoint[1][routePointCnt]);

        // reset vars //
        gpsPoint[0] = 0;
        gpsPoint[1] = 0;
        gpsPoint[2] = 0;
        gpsReadFlag = false;

        // Actualitzar KM ruta //
        if(routePointCnt >= 1)
        {
            gpsPnt.prevLat = routePoint[0][routePointCnt - 1];
            gpsPnt.prevLon = routePoint[1][routePointCnt - 1];

            gpsPnt.lat = routePoint[0][routePointCnt];
            gpsPnt.lon = routePoint[1][routePointCnt];

            kmsRouteNow = kmsRouteNow + mapMangr->calculate_km_route(gpsPnt);

            qDebug() << "calculate_km_route: " << kmsRouteNow;

            ////////////////////////////////
            // calcular conta kms COCKPIT //
            cntKmsCokpit = kmsRouteNow - initRouteKms + initCockpitKms;
            ui->kmLabel->setText(QString::number(cntKmsCokpit, 'f', 1));

            // Guradar log de velocitat //
            //spMangr->save_velocity_log();
        }

        // incrementar punts de ruta //
        routePointCnt++;
        routePointNum = routePointCnt;
    }

    // Contador de temps; 1 unitat = 100ms //
    saveRoutePeriode++;

    // actualitzacions cada 10 segon //
    if(saveRoutePeriode % 50 == 0)
    {
        ////////////////
        // ROUTE INFO //
        ui->infoLabel->setText(QString("%1 | Km: %2  Ele: %3m  Time: %4")
                               .arg(spMangr->accessible_dateTime().toString("HH:mm"))//hora actual
                               .arg(QString::number(kmsRouteNow, 'f', 1))//Kms
                               .arg(QString::number(routePoint[2][routePointCnt - 1]))//elevacio
                               .arg(mapMangr->calculate_time_route(initRouteV.initDateHour)));//temps de ruta

        ui->infoLabel->adjustSize();

        /////////////////////////
        // GPIO read  POWER OFF//
        // KL30 LOW //
        if(gpioMangr->gpio_read() == 1 && !pwrOffFlg)
        {
            pwrOffFlg = true;
            pwrOffTimer->start(60000 * PWR_OFF_TIME);

            qDebug() << "KL30 = LOW. System power off on 5 min";
        }
        // KL30 HIGH //
        else if(gpioMangr->gpio_read() == 2 && pwrOffFlg)
        {
            pwrOffFlg = false;
            pwrOffTimer->stop();
            qDebug() << "KL30 = HIGH. System power off aborted";
        }

        // CLAUSOR LOW //
        if(gpioMangr->gpio_read() == 3)
        {
            swkl30Flg = true;
            system_pwr_off();

            qDebug() << "Clausor = LOW. System power off NOW";
        }
    }

    // actualitzacions cada 1 minut //
    if(saveRoutePeriode == 600)
    {
        mapMangr->save_route_points();
        mapMangr->save_route_waypoints();

        configMangr->write_config_json("odometer", NULL, QString::number(cntKmsCokpit, 'f', 1));

        saveRoutePeriode = 0;
    }

}

void MainWindow::update_app_status()
{
    /////////////////////
    // Carregar config //
    configMangr->read_config_json();

    // View Tab //
    if(placesActive)
    {
        ui->sPlaces->setChecked(true);
    }

    if(fuelsActive)
    {
        ui->sFuels->setChecked(true);
    }

    if(trackInfoActive)
    {
        ui->sInfo->setChecked(true);
    }

    if(nightModeActive)
    {
        ui->sNight->setChecked(true);
        nightMode = true;
    }

    if(fullScreenActive)
    {
        ui->sFullScrn->setChecked(true);
    }

    ui->sKL30->setChecked(true);

    // Track Tab //
    if(trackActive)
    {
        open_gpx(trackName);
    }
    if(routeActive)
    {
        // carrega la ruta catual a variable //
        mapMangr->load_route_points();
        mapMangr->load_route_waypoints();

        // pintem canvas //
        for(int i = 0; i < routePointNum; i++)
            add_point_route(routePoint[0][i], routePoint[1][i], routePoint[2][i]);

        for(int i = 0; i < waypointNum; i++)
            add_waypoint(waypoint[0][i], waypoint[1][i], waypoint[2][i]);
    }

    // COCKPIT Tab //
    if(cntKms != "0.0")
    {
        ui->kmLabel->setText(cntKms);
        cntKmsCokpit = initCockpitKms = cntKms.toDouble();
    }
    else
        ui->kmLabel->setText("0.0");

    ///////////////////////////////////
    // Llegir dades de ruta iniciada //
    initRouteV = mapMangr->route_init_status();
    kmsRouteNow =initRouteKms = initRouteV.initKm;

    // ROUTE INFO //
    ui->infoLabel->setText(QString("%1 | Km: %2  Ele: %3m  Time: %4")
                           .arg(spMangr->accessible_dateTime().toString("HH:mm"))//hora actual
                           .arg(QString::number(kmsRouteNow, 'f', 1))//Kms
                           .arg(QString::number(routePoint[2][routePointCnt]))//elevacio
                           .arg(mapMangr->calculate_time_route(initRouteV.initDateHour)));//temps de ruta

    ui->infoLabel->adjustSize();
}


/////////////////////////////////////////////////////////////////////////////////
// Map Tab
/////////////////////////////////////////////////////////////////////////////////
void MainWindow::load_vector_map()
{
    // obrir mapa //
    QString uri = QString("type=mbtiles&url=%1").arg(mapPath);

    vtLayer = new QgsVectorTileLayer(uri, "MBTiles Vector");

    if (!vtLayer->isValid())
    {
        qDebug() << "MBTiles vector invalid";
        delete vtLayer;
        return;
    }
    QgsProject::instance()->addMapLayer(vtLayer);
    vtLayer->triggerRepaint();
    vtLayer->setCrs(QgsCoordinateReferenceSystem("EPSG:3857"));

    bool error;
    vtLayer->loadNamedStyle(mapStyle, error);
    if (!error)
    {
        qDebug() << "Error carregant estil de mapa:" << error;
    }

    vtLayer->triggerRepaint();
    ui->mapWidget->setLayers({ vtLayer });
    ui->mapWidget->setDestinationCrs(vtLayer->crs());
    ui->mapWidget->setExtent(vtLayer->extent());
    ui->mapWidget->refresh();
}


void MainWindow::on_centerBtn_clicked()
{  
    if(routePoint[0][routePointCnt - 1] > 0 && routePointCnt > 0)
    {
        centerAuto = true;
        set_gps_position(routePoint[0][routePointCnt - 1], routePoint[1][routePointCnt - 1]);
        mCanvas->zoomScale(7000);
    }
    else
    {
        centerAuto = false;
        set_gps_position(41.3464, 1.6979); //Vilafranca
        mCanvas->zoomScale(45000);
    }
//    set_gps_position(41.3464, 1.6979); //Vilafranca
//    set_gps_position(41.3424, 1.7214); // Sta. Maria dels Horts Vilafranca
}


void MainWindow::on_zoomInBtn_clicked()
{
    if(mCanvas->scale() > 900)
    {
        mCanvas->zoomIn();
        mCanvas->refresh();
    }
}


void MainWindow::on_zoomOutBtn_clicked()
{
    if(mCanvas->scale() < 7.68e+06)
    {
        mCanvas->zoomOut();
        mCanvas->refresh();
    }
}

void MainWindow::set_gps_position(float lat, float lon)
{

//    mMarker->setVisible(true);

    if(nightMode)
        mMarker->setColor(Qt::white);
    else
        mMarker->setColor(Qt::blue);

    mMarker->setIconType(QgsVertexMarker::ICON_TRIANGLE);
    mMarker->setIconSize(20);
    mMarker->setPenWidth(3);

    QgsCoordinateTransform transform(
        QgsCoordinateReferenceSystem("EPSG:4326"),
        ui->mapWidget->mapSettings().destinationCrs(),
        QgsProject::instance()
    );

    QgsPointXY point = transform.transform(QgsPointXY(lon, lat));

//    // Resultat en metres
//    qDebug() << "X:" << point.x()
//             << "Y:" << point.y();

//    qDebug() << "caching" << mCanvas->isCachingEnabled();
//    qDebug() << "parallel" << mCanvas->isParallelRenderingEnabled();
//    qDebug() << "opaque" << mCanvas->testAttribute(Qt::WA_OpaquePaintEvent);
//    qDebug() << "Sys bckGnd" << mCanvas->testAttribute(Qt::WA_NoSystemBackground);

    // refresc del mapa, mode auto o manual //
    if(centerAuto)
    {
        mMarker->setVisible(true);

        mMarker->setCenter(point);
        ui->mapWidget->setCenter(point);
        ui->mapWidget->setRotation(routeAngle);
        ui->mapWidget->refresh();
        ui->trackWidget->refresh();
    }
    else
    {       
        mMarker->setVisible(false);

        ui->mapWidget->setCenter(point);
        ui->mapWidget->setRotation(0);
        ui->mapWidget->refresh();
        ui->trackWidget->refresh();
    }
}

double MainWindow::map_rotation(float lat1, float lon1, float lat2, float lon2)
{
    const double R = 6378137.0;
    auto lonToX = [R](double lon) { return lon * M_PI / 180.0 * R; };
    auto latToY = [R](double lat) { return R * std::log(std::tan(M_PI/4.0 + lat * M_PI / 360.0)); };

    double x1 = lonToX(lon1);
    double y1 = latToY(lat1);
    double x2 = lonToX(lon2);
    double y2 = latToY(lat2);

    double dx = x2 - x1;
    double dy = y2 - y1;

    // Suavitzar gir amb angle min //
    if (std::hypot(dx, dy) < rotateMinAngle)// org 0.5
        return lastAngle;

    // Corregido para que 0° apunte hacia arriba //
    double angle = std::atan2(dy, dx) * 180.0 / M_PI - 90.0;
    if (angle < 0)
        angle += 360.0;

    // Suavizado vectorial //
    double alpha = 0.2;
    double radNew  = angle * M_PI / 180.0;
    double radLast = lastAngle * M_PI / 180.0;

    double x = std::sin(radLast) * (1.0 - alpha) + std::sin(radNew) * alpha;
    double y = std::cos(radLast) * (1.0 - alpha) + std::cos(radNew) * alpha;

    double smoothAngle = std::atan2(x, y) * 180.0 / M_PI;
    if (smoothAngle < 0)
        smoothAngle += 360.0;

    lastAngle = smoothAngle;
    return smoothAngle;
}

void MainWindow::set_route_layer()
{
    ///////////////////////////////
    // crear capa ruta real time //
    QgsVectorLayer* routeLayer = new QgsVectorLayer(
        "LineStringZ?crs=EPSG:4326",
        "routeRealTime",
        "memory"
    );

    if (!routeLayer->isValid())
    {
        qDebug() << "Capa inválida:" << routeLayer->name();
        delete routeLayer;
        return;
    }

    // Afegir style //
    bool error;
    routeLayer->loadNamedStyle(gpxRoutesStyle, error);

    // Afegir projecte a QGIS //
    QgsProject::instance()->addMapLayer(routeLayer);

    // Guardem ID //
    routeLayerRtId = routeLayer->id();
//    qDebug() << "routeLayerRtId:" << routeLayerRtId;

    // Obtenir capes actuales //
    QList<QgsMapLayer*> rLayers = ui->mapWidget->layers();

    rLayers.insert(0, routeLayer);

    ui->mapWidget->setDestinationCrs(QgsCoordinateReferenceSystem("EPSG:3857"));

    // Aplicar capes //
    ui->mapWidget->setLayers(rLayers);
    ui->mapWidget->refresh();

    ui->trackWidget->setLayers(rLayers);
    ui->trackWidget->refresh();

    ////////////////////////////////////
    // crear capa waypoints real time //
    QgsVectorLayer* waypointLayer = new QgsVectorLayer(
        "PointZ?crs=EPSG:4326",
        "WaypointRealTime",
        "memory"
    );

    if (!waypointLayer->isValid())
    {
        qDebug() << "Capa inválida:" << waypointLayer->name();
        delete waypointLayer;
        return;
    }

    // Afegir style //
    waypointLayer->loadNamedStyle(gpxWaypointsStyle, error);

    // Afegir projecte a QGIS //
    QgsProject::instance()->addMapLayer(waypointLayer);

    // Guardem ID //
    waypointLayerRtId = waypointLayer->id();
//    qDebug() << "waypointRtId:" << waypointLayerRtId;

    // Obtenir capes actuales //
    QList<QgsMapLayer*> wLayers = ui->mapWidget->layers();

    wLayers.insert(0, waypointLayer);

    ui->mapWidget->setDestinationCrs(QgsCoordinateReferenceSystem("EPSG:3857"));

    // Aplicar capes //
    ui->mapWidget->setLayers(wLayers);
    ui->mapWidget->refresh();

    ui->trackWidget->setLayers(wLayers);
    ui->trackWidget->refresh();

    ///////////////////
    // update config //
    configMangr->write_config_json("route_active", true, NULL);
}

void MainWindow::add_point_route(float lat, float lon, float ele)
{
    // Recuperar capa desde QgsProject //
    QgsMapLayer* baseLayer = QgsProject::instance()->mapLayer(routeLayerRtId);

    if(!baseLayer)
        return;

    QgsVectorLayer* routeLayer = qobject_cast<QgsVectorLayer*>(baseLayer);

    if(!routeLayer)
        return;

    routePoints.append(QgsPoint(lon, lat, ele));

    QgsGeometry geom = QgsGeometry::fromPolyline(routePoints);
    QgsVectorDataProvider* provider = routeLayer->dataProvider();

    if (routeFid == -1)
    {
        QgsFeature routeFeature(routeLayer->fields());
        routeFeature.setGeometry(geom);
        routeFeature.setAttributes({1});

        QgsFeatureList features;
        features.append(routeFeature);

        provider->addFeatures(features);
        routeFid = features.first().id();
    }
    else
    {
        QMap<QgsFeatureId, QgsGeometry> geomMap;
        geomMap.insert(routeFid, geom);
        provider->changeGeometryValues(geomMap);
    }

    if(centerAuto)
    {
        routeLayer->updateExtents();
        routeLayer->triggerRepaint();
    }
//    ui->trackWidget->refresh();
//    ui->mapWidget->refresh();

//    qDebug() << "add_point_route: " << lat << lon << ele;

}


void MainWindow::add_waypoint(float lat, float lon, float ele)
{
    QgsMapLayer* baseLayer = QgsProject::instance()->mapLayer(waypointLayerRtId);
    if (!baseLayer)
        return;

    QgsVectorLayer* waypointLayer = qobject_cast<QgsVectorLayer*>(baseLayer);
    if (!waypointLayer)
        return;

    if (!waypointLayer->isValid())
        return;

    // Crear geometria //
    QgsGeometry geom = QgsGeometry::fromPointXY(QgsPointXY(lon, lat));

    QgsFeature feature(waypointLayer->fields());
    feature.setGeometry(geom);

    feature.setAttribute("elevation", ele);
    feature.setAttribute("timestamp", routeDateHour << spMangr->accessible_dateTime().toString("yyyy-MM-ddTHH:mm:ssZ"));
    QgsVectorDataProvider* provider = waypointLayer->dataProvider();

    QgsFeatureList list;
    list.append(feature);

    provider->addFeatures(list);

    waypointLayer->updateExtents();
    waypointLayer->triggerRepaint();

    // refrscar //
    ui->mapWidget->refresh();
    ui->trackWidget->refresh();

    qDebug() << "Waypoint añadido:" << lat << lon;

}

/////////////////////////////////////////////////////////////////////////////////
// Track Tab
/////////////////////////////////////////////////////////////////////////////////

void MainWindow::on_gpxOpenBtn_clicked()
{
    // GPX seleccionat //
    QModelIndex index = ui->gpxListView->currentIndex();
    if (!index.isValid())
        return;

    // guardar index listView //
    trackListViewIndex = index.row();

    // obrir GPX //
    open_gpx(gpxFiles[index.row()]);
}

void MainWindow::on_gpxQuitBtn_clicked()
{
    if(tracksLayerId[0] != "")
    {
        // Borrar grafic //
        if (chartView) {
            delete chartView;
            chartView = nullptr;
            chartG = nullptr;
        }
        // borrar capes dels tracks //
        for(int i = 0; i < trackCnt; i++)
        {
            QgsProject::instance()->removeMapLayer(tracksLayerId[i]);
            QgsProject::instance()->removeMapLayer(routesLayerId[i]);
            QgsProject::instance()->removeMapLayer(waypointsLayerId[i]);

            tracksLayerId[i] = "";
            routesLayerId[i] = "";
            waypointsLayerId[i] = "";
        }

        // Reset track vars //
        trackPointCnt = 0;
        trackPointNum = 0;
        maxElevation = 0;
        kmTotal = 0;
        trackCnt = 0;

        // Borrar labels //
        ui->trackNameLabel->clear();
        ui->trackInfoLabel->clear();

        // Refrescar //
        ui->mapWidget->refresh();
        ui->trackWidget->refresh();

        // update config //
        configMangr->write_config_json("track_active", false, NULL);
        configMangr->write_config_json("track_name", NULL, "");
    }
}

void MainWindow::on_waypointBtn_clicked()
{
    if(routePoint[0][routePointCnt -1] != 0)
    {
        // copiem coordenada actual a waypoint //
        waypoint[0][waypointCnt] = routePoint[0][routePointCnt - 1]; // Lon
        waypoint[1][waypointCnt] = routePoint[1][routePointCnt - 1]; // Lat
        waypoint[2][waypointCnt] = routePoint[2][routePointCnt - 1]; // Ele


        add_waypoint(waypoint[0][waypointCnt], waypoint[1][waypointCnt], waypoint[2][waypointCnt]);
        waypointDateHour << spMangr->accessible_dateTime().toString("yyyy-MM-ddTHH:mm:ssZ");

        waypointCnt++;
        waypointNum++;
    }
}


void MainWindow::on_saveRouteBtn_clicked()
{
    // Missatge de confirmacio save/Finish route //
    QMessageBox msgBox;
    msgBox.setWindowTitle("Save Route");
    msgBox.setText("Are you sure you want to SAVE and FINISH route?");
    msgBox.setIcon(QMessageBox::Question);

    // Botons //
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);

    // treure capçalera //
    msgBox.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    // Estil //
    msgBox.setStyleSheet(R"(
        QMessageBox
        {
            background:  rgba(100, 100, 100, 120);
            /*border-radius: 15px;*/
            /*padding: 15px;*/
        }
        QLabel { color: white; font-size: 14px; }
        QPushButton
        {
            width: 80px;
            height: 30px;
            background-color: #3498db;
            color: white;
            border: none;
            border-radius: 10px;
        }
        QPushButton:pressed
        {
            background-color: #2e86c1;
            padding-top: 2px;
            padding-left: 2px;
        }
    )");

    // Executa //
    int ret = msgBox.exec();

    // Borrar routa //
    if (ret == QMessageBox::Ok)
        ;
    else
        return;

    // Save route //
    if(routePointNum <= 0)
    {
        qDebug() << "ERROR ruta sense coordenades";
        return;
    }

    QString outputPath = gpxDirPath + "track_" + spMangr->accessible_dateTime().toString("dd-MM-yyyy_HH:mm:ss") + ".gpx";

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    /////////
    // GPX //
    xml.writeStartElement("gpx");
    xml.writeAttribute("version", "1.1");
    xml.writeAttribute("creator", "Control Xic maps");
    xml.writeDefaultNamespace("http://www.topografix.com/GPX/1/1");

    //////////////
    // METADATA //
    xml.writeStartElement("metadata");

    xml.writeTextElement("name", "track_" + spMangr->accessible_dateTime().toString("dd-MM-yyyy_HH:mm:ss"));


    xml.writeStartElement("author");
    xml.writeTextElement("name", "Control Xic maps");
    xml.writeEndElement(); // author

    xml.writeEndElement(); // metadata

    ///////////////
    // WAYPOINTS //
    for (int i = 0; i < waypointNum; ++i)
    {
        xml.writeStartElement("wpt");

        xml.writeAttribute("lat", QString::number(waypoint[0][i], 'f', 8));
        xml.writeAttribute("lon", QString::number(waypoint[1][i], 'f', 8));

        xml.writeTextElement("ele",
            QString::number(waypoint[2][i], 'f', 2));

        if (i < waypointDateHour.size())
            xml.writeTextElement("time", waypointDateHour.at(i));

        xml.writeEndElement(); // wpt
    }

    ///////////
    // TRACK //
    xml.writeStartElement("trk");
    xml.writeTextElement("name", "Track");

    xml.writeStartElement("trkseg");

    for(int i = 0; i < routePointNum; i++)
    {
        xml.writeStartElement("trkpt");

        // 🔥 SIEMPRE lat primero
        xml.writeAttribute("lat",
            QString::number(routePoint[0][i], 'f', 15));

        xml.writeAttribute("lon",
            QString::number(routePoint[1][i], 'f', 15));

        xml.writeTextElement("ele",
            QString::number(routePoint[2][i], 'f', 2));

        if(i < routeDateHour.size())
            xml.writeTextElement("time", routeDateHour.at(i));

        xml.writeEndElement(); // trkpt
    }

    xml.writeEndElement(); // trkseg
    xml.writeEndElement(); // trk
    xml.writeEndElement(); // gpx

    xml.writeEndDocument();
    file.close();

    qDebug() << "GPX creat correctament amb lat lon ordenats";

    // update config //
    configMangr->write_config_json("route_active", false, NULL);

    // borrar route layer //
    reset_route();

    // gpx refresca lisView //
    get_gpx_files();
}



void MainWindow::on_resetRouteBtn_clicked()
{
    // Missatge de confirmacio reset route //
    QMessageBox msgBox;
    msgBox.setWindowTitle("Delete Route");
    msgBox.setText("Are you sure you want to DELETE route?");
    msgBox.setIcon(QMessageBox::Question);

    // Botons //
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);

    // treure capçalera //
    msgBox.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    // Estil //
    msgBox.setStyleSheet(R"(
        QMessageBox
        {
            background:  rgba(100, 100, 100, 120);
            /*border-radius: 15px;*/
            /*padding: 15px;*/
        }
        QLabel { color: white; font-size: 14px; }
        QPushButton
        {
            width: 80px;
            height: 30px;
            background-color: #3498db;
            color: white;
            border: none;
            border-radius: 10px;
        }
        QPushButton:pressed
        {
            background-color: #2e86c1;
            padding-top: 2px;
            padding-left: 2px;
        }
    )");

    // Executa //
    int ret = msgBox.exec();

    // Borrar routa //
    if (ret == QMessageBox::Ok) {
        reset_route();
    }
    else
    {
        return;
    }
}


void MainWindow::reset_route()
{
    ////////////////////////
    // borrar route layer //
    QgsProject::instance()->removeMapLayer(routeLayerRtId);
    routeLayerRtId = "";
    routeDateHour.clear();
    routeFid = -1;
    routePoints.clear();
    initRouteV.initKm = 0;
    kmsRouteNow = 0;
    initRouteV.initDateHour = QDateTime();// = 0

    for(int i = 0; i < routePointNum; i++)
    {
        routePoint[0][i] = 0;
        routePoint[1][i] = 0;
        routePoint[2][i] = 0;
    }

    routePointNum = 0;
    routePointCnt = 0;

    if (QFile::remove("/home/orangepi/00_controlXic/Qt-Projects/c_x_maps/01_FILES/ROUTE/current_route.txt"))
    {
        qDebug() << "Arxiu current_route.txt eliminat correctament";
    }
    else
    {
        qDebug() << "Error al eliminar arxiu current_route.txt";
    }

    ///////////////////////////
    // borrar waypoint layer //
    QgsProject::instance()->removeMapLayer(waypointLayerRtId);
    waypointLayerRtId = "";
    waypointDateHour.clear();

    for(int i = 0; i < waypointNum; i++)
    {
        waypoint[0][i] = 0;
        waypoint[1][i] = 0;
        waypoint[2][i] = 0;
    }

    waypointNum = 0;

    if (QFile::remove("/home/orangepi/00_controlXic/Qt-Projects/c_x_maps/01_FILES/ROUTE/current_waypoint.txt"))
    {
        qDebug() << "Arxiu current_waypoint.txt eliminat correctament";
    }
    else
    {
        qDebug() << "Error al eliminar arxiu current_waypoint.txt";
    }

    ///////////////
    // Refrescar //
    ui->mapWidget->refresh();
    ui->trackWidget->refresh();

    // trona a crear capa de ruta y waypoint real time //
    set_route_layer();
}

void MainWindow::open_gpx(QString gpxFile)
{
    // comprova existencia de l'arxiu //
    if (!QFile::exists(gpxDirPath + gpxFile))
    {
        qDebug() << "Arxiu GPX invalid" << gpxFile;

        // update config //
        gpxFile = "";
        configMangr->write_config_json("track_active", false, NULL);
        configMangr->write_config_json("track_name", NULL, gpxFile);

        return;
    }

    QFile file(gpxFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {

    }

    // Obre com a max 9 tracks //
    if(trackCnt < 9)
    {
        clear_elevation_graphic();

        // Obrir GPX complert a mCanvas//
        QStringList layersNames = { "tracks", "routes", "waypoints" };

        for (const QString &name : layersNames)
        {
            QString uri = gpxDirPath + gpxFile + "|layername=" + name;

//            qDebug() << "uri sel: " << uri;

            QgsVectorLayer *layer = new QgsVectorLayer(uri, name, "ogr");

            if (!layer->isValid())
            {
                qDebug() << "Capa GPX invalida:" << name;
                delete layer;
                continue;
            }

            // Afegir style //
            bool error;
            if(name == "tracks")
                layer->loadNamedStyle(gpxTracksStyle, error);
            else if(name == "routes")
                layer->loadNamedStyle(gpxRoutesStyle, error);
            else if(name == "waypoints")
                layer->loadNamedStyle(gpxWaypointsStyle, error);

            // Afegir projecte a QGIS //
            QgsProject::instance()->addMapLayer(layer);

            if(name == "tracks")
                tracksLayerId[trackCnt] = layer->id();
            else if(name == "routes")
                routesLayerId[trackCnt] = layer->id();
            else if(name == "waypoints")
            {
                waypointsLayerId[trackCnt] = layer->id();

                if(trackCnt >= 9)
                    trackCnt = 9;
                else
                    trackCnt++;
            }
            // Obtenir capes actuales //
            QList<QgsMapLayer*> layers = ui->mapWidget->layers();

            // Afegir GPX sobre el mapa base //
            layers.insert(2, layer);

            // Configurar CRS del canvas //
            ui->mapWidget->setDestinationCrs(QgsCoordinateReferenceSystem("EPSG:3857"));
            ui->trackWidget->setDestinationCrs(QgsCoordinateReferenceSystem("EPSG:3857"));

            // Ajustar extensio //
            if(name == "tracks")
            {
                // map canvas //
                ui->mapWidget->setDestinationCrs(layer->crs());
                ui->mapWidget->setExtent(layer->extent());

                // track canvas //
                vtLayer->triggerRepaint();
                ui->trackWidget->setLayers({ layer });
                ui->trackWidget->setDestinationCrs(layer->crs());
                ui->trackWidget->setExtent(layer->extent());
            }

            // Aplicar capes //
            ui->trackWidget->setLayers(layers);
            ui->trackWidget->refresh();

            ui->mapWidget->setLayers(layers);
            ui->mapWidget->refresh();
        }

        // Extreu dades del GPX //
        mapMangr->get_track_settings(gpxDirPath + gpxFile);

        set_elevation_graphic();
    }

    // afegir a label GPX seleccionat //
    ui->trackNameLabel->setText(gpxFile);
    ui->trackInfoLabel->setText("Points: " + QString::number(trackPointNum) + "\t Km: " + QString::number(kmTotal, 'f', 1) + "\t Elevation max: " + QString::number(maxElevation, 'f', 1) + "m");

    // update config //
    configMangr->write_config_json("track_active", true, NULL);
    configMangr->write_config_json("track_name", NULL, gpxFile);
}

void MainWindow::clear_elevation_graphic()
{
    // Borrar grafic //
    if (chartView && tracksLayerId[0] != "") {
        delete chartView;
        chartView = nullptr;
        chartG = nullptr;
    }
}

void MainWindow::set_elevation_graphic()
{
    // Importar  dades del grafic //
    QLineSeries *series0 = new QLineSeries;
    QLineSeries *series1 = new QLineSeries;

    for (int i = 0; i < trackPointNum; ++i)
        series0->append(i, trackPoint[2][i]);

    *series1 << QPointF(0, 0) << QPointF(trackPointNum, 0);

    // Afegir dades del grafic //
    QAreaSeries *series = new QAreaSeries(series0, series1);

    // Estil //
    QPen pen(0xF5D65F);
    pen.setWidth(2);
    series->setPen(pen);

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, 0xF58027);
    gradient.setColorAt(1.0, 0xF5D65F);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    series->setBrush(gradient);

    // CHART //
    chartG = new QChart;
    chartG->addSeries(series);
    chartG->legend()->hide();
    chartG->createDefaultAxes();

    auto *axisX = qobject_cast<QValueAxis*>(chartG->axisX());
    auto *axisY = qobject_cast<QValueAxis*>(chartG->axisY());

    axisX->setRange(0, trackPointNum);
    axisY->setRange(0, maxElevation);

    chartG->setBackgroundVisible(false);
    chartG->setPlotAreaBackgroundVisible(false);
    chartG->setMargins(QMargins(-30, -10, -10, -25));
    chartG->setAnimationOptions(QChart::SeriesAnimations);

    // CHART VIEW //
    chartView = new QChartView(chartG);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent");
    chartView->setMouseTracking(true);

    // Treure malla, exios i labels //
    for (QAbstractAxis *axis : chartG->axes()) {
        axis->setGridLineVisible(false);
        axis->setLineVisible(false);
        axis->setLabelsVisible(false);
    }

    // registrar mouse a event filter //
    chartView->viewport()->installEventFilter(this);

    // Netejar LAYOUT ANTERIOR //
    delete ui->eleWidget->layout();

    // Afegir a Widged //
    QVBoxLayout *layout = new QVBoxLayout(ui->eleWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);

    // LINEA VERTICAL //
    m_line = new QGraphicsLineItem(chartG);
    m_line->setPen(QPen(Qt::red, 1, Qt::DashLine));
    m_line->setZValue(1000);
    m_line->setVisible(false);
    chartG->scene()->addItem(m_line);

    // TEXT //
    m_text = new QGraphicsSimpleTextItem(chartG);
    if(!nightMode)
        m_text->setBrush(Qt::black);
    else
        m_text->setBrush(Qt::white);
    m_text->setZValue(1001);
    m_text->setVisible(false);
    chartG->scene()->addItem(m_text);
}

// Deteccio events del mouse //
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Deteccio del mouse al mapa Canvas //
    if(ui->tabWidget->currentIndex() == 0 && obj == mCanvas->viewport() && event->type() == QEvent::MouseButtonPress)
    {
        qDebug() << "mouse canvas";
//        movManual = true;
        centerAuto = false;
    }

    // Deteccio del mouse a chartG //
    if(ui->tabWidget->currentIndex() == 1)
    {
        if (!chartG || !m_line || !m_text)
            return false;

        if(event->type() == QEvent::MouseMove)
        {
//            qDebug() << "mouse chartG";

            auto *mouse = static_cast<QMouseEvent*>(event);

            QPointF value = chartG->mapToValue(mouse->pos());
            QRectF plotArea = chartG->plotArea();

            auto *axisX = qobject_cast<QValueAxis*>(chartG->axisX());
            if (!axisX)
                return false;

            if (value.x() < axisX->min() || value.x() > axisX->max())
            {
                m_line->setVisible(false);
                m_text->setVisible(false);
                return false;
            }

            qreal x = chartG->mapToPosition(QPointF(value.x(), 0)).x();

            m_line->setLine(x, plotArea.top(), x, plotArea.bottom());
            m_line->setVisible(true);

            m_text->setText(QString("%1m\n%2km")
                .arg((trackPoint[2][(uint16_t)value.x()]), 0, 'f', 2)
                .arg((trackPoint[3][(uint16_t)value.x()]), 0, 'f', 2));

            m_text->setPos(10, 10);
            m_text->setVisible(true);
            trackMarker->setVisible(true);

            set_track_position(true, (trackPoint[0][(uint32_t)value.x()]), (trackPoint[1][(uint32_t)value.x()]));

            return true;
        }
        if (event->type() == QEvent::Leave)
        {
            m_line->setVisible(false);
            m_text->setVisible(false);
            trackMarker->setVisible(false);
        }
    }

    return false;
}

void MainWindow::set_track_position(bool icon, float lat, float lon)
{
    if(nightMode)
        trackMarker->setColor(Qt::white);
    else
        trackMarker->setColor(Qt::blue);

    trackMarker->setIconType(QgsVertexMarker::ICON_TRIANGLE);
    trackMarker->setIconSize(20);
    trackMarker->setPenWidth(3);

    QgsCoordinateTransform transform(
        QgsCoordinateReferenceSystem("EPSG:4326"),
        ui->trackWidget->mapSettings().destinationCrs(),
        QgsProject::instance()
    );

    QgsPointXY point = transform.transform(QgsPointXY(lon, lat));

    trackMarker->setCenter(point);
    ui->trackWidget->refresh();
}

void MainWindow::get_gpx_files()
{
    QDir dir(gpxDirPath);

    QStringList filters;
    filters << "*.gpx";

    gpxFiles = dir.entryList(
        filters,
        QDir::Files | QDir::NoSymLinks
    );

//    qDebug() << "files: " << gpxFiles[0];
//    qDebug() << "files: " << gpxFiles[1];

    QStringListModel *model = new QStringListModel(gpxFiles, this);
    ui->gpxListView->setModel(model);
}



/////////////////////////////////////////////////////////////////////////////////
// Config Tab
/////////////////////////////////////////////////////////////////////////////////
void MainWindow::toggle_view_places(bool status)
{
    if(status)
    {
        // Obrir noms dels pobles //
        QString uri = placesPath + "|layername=" + "spainPlaces";

        QgsVectorLayer *layer = new QgsVectorLayer(uri, "spainPlaces", "ogr");

        if (!layer->isValid())
        {
            qDebug() << "Capa invalida:" << "spainPlaces";
            delete layer;
        }

        bool error;
        layer->loadNamedStyle(placesStyle, error);

        if (!error)
            qDebug() << "Error cargando el estilo";

        // Afegir projecte a QGIS //
        QgsProject::instance()->addMapLayer(layer);
        placesLayerId = layer->id();

        // Obtenir capes actuales //
        QList<QgsMapLayer*> layers = ui->mapWidget->layers();

        // Afegir capa sobre el mapa base //
        layers.insert(2, layer);

        // Configurar CRS del canvas //
        ui->mapWidget->setDestinationCrs(QgsCoordinateReferenceSystem("EPSG:3857"));

        // Aplicar capes //
        ui->mapWidget->setLayers(layers);
        ui->mapWidget->refresh();

        // update config //
        configMangr->write_config_json("places_active", true, NULL);
    }
    else
    {
        // Borrar capa noms ciutats //
        QgsProject::instance()->removeMapLayer(placesLayerId);

        // Configurar CRS del canvas //
        ui->mapWidget->setDestinationCrs(QgsCoordinateReferenceSystem("EPSG:3857"));

        // Refrescar //
        ui->mapWidget->refresh();

        // update config //
        configMangr->write_config_json("places_active", false, NULL);
    }
}

void MainWindow::toggle_view_fuels(bool status)
{
    if(status)
    {
        // Obrir noms dels pobles //
        QString uri = fuelsPath + "|layername=" + "spainFuels";

        QgsVectorLayer *layer = new QgsVectorLayer(uri, "spainFuels", "ogr");

        if (!layer->isValid())
        {
            qDebug() << "Capa invalida:" << "spainFuels";
            delete layer;
        }

        bool error;
        layer->loadNamedStyle(fuelsStyle, error);

        if (!error)
            qDebug() << "Error cargando el estilo";

        // Afegir projecte a QGIS //
        QgsProject::instance()->addMapLayer(layer);
        fuelsLayerId = layer->id();

        // Obtenir capes actuales //
        QList<QgsMapLayer*> layers = ui->mapWidget->layers();

        // Afegir capa sobre el mapa base //
        layers.insert(2, layer);

        // Configurar CRS del canvas //
        ui->mapWidget->setDestinationCrs(QgsCoordinateReferenceSystem("EPSG:3857"));

        // Aplicar capes //
        ui->mapWidget->setLayers(layers);
        ui->mapWidget->refresh();

        // update config //
        configMangr->write_config_json("fuels_active", true, NULL);
    }
    else
    {
        // Borrar Layer gasolineres //
        QgsProject::instance()->removeMapLayer(fuelsLayerId);

        // Refrescar //
        ui->mapWidget->refresh();

        // update config //
        configMangr->write_config_json("fuels_active", false, NULL);
    }
}

void MainWindow::toggle_view_infoLabel(bool status)
{
    if(status)
    {
        ui->infoLabel->setVisible(true);

        // update config //
        configMangr->write_config_json("trackInfo_active", true, NULL);
    }
    else
    {
        ui->infoLabel->setVisible(false);

        // update config //
        configMangr->write_config_json("trackInfo_active", false, NULL);
    }
}

void MainWindow::toggle_view_nightMode(bool status)
{
    if(status)
    {
        nightMode = true;

        ui->centralwidget->setStyleSheet(R"(
            background-color: #2E2F30;
            color: white;
        )");
        mCanvas->setCanvasColor(QColor("#2E2F30"));
        trackCanvas->setCanvasColor(QColor("#2E2F30"));
        ui->gpxListView->setStyleSheet(R"(
            QListView {
                    background:  rgba(100, 100, 100, 120);
                    border-radius: 10px;
                    padding: 4px;
                    color: white;
                    font-size: 14px;
            }
            QListView::item
            {
                    padding: 4px 12px;
                    margin: 4px 2px;
                    border-radius: 10px;
            }
            QListView::item:selected
            {
                    background: #3498db;
                    color: white;
            }
        )");

        ui->velLavel->setStyleSheet(R"(
            background-color:  rgba(0, 0, 0, 0);
            color: white;
            font-size: 220px;
            font-weight: bold;
            border-radius: 80px;
        )");

        ui->label_8->setStyleSheet(R"(
            background-color:  rgba(0, 0, 0, 0);
            color: white;
            font-size: 20px;
            font-weight: bold;
            border-radius: 80px;
        )");

        ui->label_12->setStyleSheet(R"(
            color: white;
            font-size: 20px;
            font-weight: bold;
            border-radius: 80px;
        )");

        ui->kmLabel->setStyleSheet(R"(
            color: white;
            font-size: 50px;
            font-weight: bold;
            border-radius: 80px;
        )");

        // update config //
        configMangr->write_config_json("nightMode_active", true, NULL);
    }
    else
    {
        nightMode = false;

        ui->centralwidget->setStyleSheet(R"(
            background-color: #EFEFEF;
            color: black;
        )");
        mCanvas->setCanvasColor(QColor("#FBFBFB"));
        trackCanvas->setCanvasColor(QColor("#FBFBFB"));
        ui->gpxListView->setStyleSheet(R"(
            QListView
            {
                    background:  rgba(100, 100, 100, 120);
                    border-radius: 10px;
                    padding: 4px;
                    color: black;
                    font-size: 14px;
            }
            QListView::item
            {
                    padding: 4px 12px;
                    margin: 4px 2px;
                    border-radius: 10px;
            }
            QListView::item:selected
            {
                    background: #3498db;
                    color: white;
            }
        )");

        ui->velLavel->setStyleSheet(R"(
            background-color:  rgba(0, 0, 0, 0);
            color: black;
            font-size: 220px;
            font-weight: bold;
            border-radius: 80px;
        )");

        ui->label_8->setStyleSheet(R"(
            background-color:  rgba(0, 0, 0, 0);
            color: black;
            font-size: 20px;
            font-weight: bold;
            border-radius: 80px;
        )");

        ui->label_12->setStyleSheet(R"(
            color: black;
            font-size: 20px;
            font-weight: bold;
            border-radius: 80px;
        )");

        ui->kmLabel->setStyleSheet(R"(
            color: black;
            font-size: 50px;
            font-weight: bold;
            border-radius: 80px;
        )");

        // update config //
        configMangr->write_config_json("nightMode_active", false, NULL);
    }
}

void MainWindow::toggle_view_wifi(bool status)
{
    if(status)
    {
        // Open Server //
        fServer->open_wifi();
        fServer->start_file_server();

        // Missatge info open server //
        QMessageBox msgBox;
        msgBox.setWindowTitle("GPX Server");
        msgBox.setText("GPX Server is opened");
        msgBox.setIcon(QMessageBox::Information);

        // Botons //
        msgBox.setStandardButtons(QMessageBox::Ok );

        // treure capçalera //
        msgBox.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

        // Estil //
        msgBox.setStyleSheet(R"(
            QMessageBox
            {
                background:  rgba(100, 100, 100, 120);
            }
            QLabel { color: white; font-size: 14px; }
            QPushButton
            {
                width: 80px;
                height: 30px;
                background-color: #3498db;
                color: white;
                border: none;
                border-radius: 10px;
            }
            QPushButton:pressed
            {
                background-color: #2e86c1;
                padding-top: 2px;
                padding-left: 2px;
            }
        )");

        // Executa //
        int ret = msgBox.exec();

        // Borrar routa //
        if (ret == QMessageBox::Ok)
            ;
        else
            return;
    }
    else
    {
        // Close Server //
        fServer->stop_file_server();
        fServer->close_wifi();

        // Missatge de info close server //
        QMessageBox msgBox;
        msgBox.setWindowTitle("GPX Server");
        msgBox.setText("GPX Server is closed");
        msgBox.setIcon(QMessageBox::Information);

        // Botons //
        msgBox.setStandardButtons(QMessageBox::Ok );

        // treure capçalera //
        msgBox.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

        // Estil //
        msgBox.setStyleSheet(R"(
            QMessageBox
            {
                background:  rgba(100, 100, 100, 120);
            }
            QLabel { color: white; font-size: 14px; }
            QPushButton
            {
                width: 80px;
                height: 30px;
                background-color: #3498db;
                color: white;
                border: none;
                border-radius: 10px;
            }
            QPushButton:pressed
            {
                background-color: #2e86c1;
                padding-top: 2px;
                padding-left: 2px;
            }
        )");

        // Executa //
        int ret = msgBox.exec();

        // Borrar routa //
        if (ret == QMessageBox::Ok)
            ;
        else
            return;
    }
}

void MainWindow::toggle_view_fullScrn(bool status)
{
    if(status)
    {
        this->showFullScreen();
        configMangr->write_config_json("fullScreen_active", true, NULL);
    }
    else
    {
        this->showNormal();
        configMangr->write_config_json("fullScreen_active", false, NULL);
    }
}

void MainWindow::toggle_view_kl30(bool status)
{
    if(status)
    {
        swkl30Flg = true;
    }
    else
    {
        swkl30Flg = false;
    }
}

void MainWindow::on_cntRotationBtn_clicked()
{
    ui->mapWidget->setRotation(0);
    ui->mapWidget->refresh();
    ui->trackWidget->refresh();
}

void MainWindow::on_cntCatBtn_clicked()
{
    centerAuto = false;
    center_in_coord(41.7983, 1.5250); //Catalunya
    mCanvas->zoomScale(4000000);
}


void MainWindow::on_cntHomeBtn_clicked()
{
    centerAuto = false;
    center_in_coord(41.3464, 1.6979); //Vilafranca
    mCanvas->zoomScale(500000);
}


void MainWindow::on_rmMarkerBtn_clicked()
{
    mMarker->setVisible(false);
    ui->mapWidget->refresh();
}


void MainWindow::on_tabWidget_currentChanged(int index)
{
    if(index == 0)
        ;
    else if(index == 1)
    {
        get_gpx_files();
        ui->trackWidget->refresh();
    }
    else if(index == 2)
        ;
}

void MainWindow::center_in_coord(float lat, float lon)
{
    QgsCoordinateTransform transform(
        QgsCoordinateReferenceSystem("EPSG:4326"),
        ui->mapWidget->mapSettings().destinationCrs(),
        QgsProject::instance()
    );
    QgsPointXY point = transform.transform(QgsPointXY(lon, lat));

    ui->mapWidget->setCenter(point);
    ui->mapWidget->setRotation(0);
    ui->mapWidget->refresh();
}

///////////////////////////
// KL30 System POWER OFF //
void MainWindow::system_pwr_off()
{
    if(swkl30Flg)
    {
        // Quit app //
        qDebug() << "Quit App!!!!!";
        if(routePointNum > 0)
        {
            mapMangr->save_route_points();
            mapMangr->save_route_waypoints();
        }
        fServer->stop_file_server();
        fServer->close_wifi();

        sysPwrOffPrcss->start("bash", QStringList() << "-c" << "sudo shutdown -h +0");

        delete mCanvas;
        delete trackCanvas;
        delete mMarker;
        delete trackMarker;
        mMarker = nullptr;
        trackMarker = nullptr;
        delete ui;
    }
    else
    {
        // restart timeout //
        swkl30Flg = true;
        ui->sKL30->setChecked(true);
        qDebug() << "Restart Timer! Quit App";
        pwrOffTimer->stop();
        pwrOffFlg = true;
        pwrOffTimer->start(60000 * PWR_OFF_TIME);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// COCKPIT Tab
/////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_rstKmBtn_clicked()
{
    ui->kmLabel->setText("0.0");
    configMangr->write_config_json("odometer", NULL, "0.0");
    initCockpitKms = 0.0;
    cntKms = "0.0";
    // reset km gasolina/acumulats
}


/////////////////////////////////////////////////////////////////////////////////
// Test Tab
/////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_startTestBtn_clicked()
{
    set_gps_position(trackPoint[0][trackPointCnt], trackPoint[1][trackPointCnt]);
    gpsPosTimer->start(1000);
//    centerAuto = true;
//    movManual = false;
}

void MainWindow::on_stopTestBtn_clicked()
{
    // parar simulacio //
    gpsPosTimer->stop();
//    centerAuto = false;
    trackPointCnt = 0;
    routePointCnt = 0;

    QList<QgsMapLayer*> layers = ui->mapWidget->layers();

    // test //
    vtLayer->triggerRepaint();
    ui->mapWidget->setLayers(layers);
    ui->mapWidget->setDestinationCrs(vtLayer->crs());
    ui->mapWidget->refresh();
}

// simulacio amb el timer, track i gps //
void MainWindow::simulate_route_points()
{
    //////////////////
    // Simular ruta //
    if(trackPoint[0][trackPointCnt] > 0)
    {
        // copiem track a ruta //
        routePoint[0][routePointCnt] = trackPoint[0][trackPointCnt]; // Lon
        routePoint[1][routePointCnt] = trackPoint[1][trackPointCnt]; // Lat
        routePoint[2][routePointCnt] = trackPoint[2][trackPointCnt]; // Ele

        // pintem canvas //
        add_point_route(routePoint[0][routePointCnt], routePoint[1][routePointCnt], routePoint[2][routePointCnt]);
        routeDateHour << spMangr->accessible_dateTime().toString("yyyy-MM-ddTHH:mm:ssZ");

        // centrem gps i cursor //
        if(routePointCnt > 1 && routePointCnt <= trackPointNum)
        {
            routeAngle = map_rotation(routePoint[0][routePointCnt - 1], routePoint[1][routePointCnt - 1], routePoint[0][routePointCnt], routePoint[1][routePointCnt]);
        }

        set_gps_position(routePoint[0][routePointCnt], routePoint[1][routePointCnt]);

        // calcul de velocitat //
        spMangr->update_gps_velocity(routePoint[0][routePointCnt], routePoint[1][routePointCnt]);

        trackPointCnt++;
        routePointCnt++;

        routePointNum = routePointCnt;
    }
    else
        on_stopTestBtn_clicked();
}

void MainWindow::on_clrGpsBtn_clicked()
{
    ui->gpsTextEdit->clear();
}

void MainWindow::on_quitBtn_clicked()
{
    qDebug() << "Quit App!!!!!";
    if(routePointNum > 0)
    {
        mapMangr->save_route_points();
        mapMangr->save_route_waypoints();
    }
    fServer->stop_file_server();
    fServer->close_wifi();

    delete mCanvas;
    delete trackCanvas;
    delete mMarker;
    delete trackMarker;
    mMarker = nullptr;
    trackMarker = nullptr;
    delete ui;
}


void MainWindow::on_pwerOffBtn_clicked()
{
    // Quit app //
    qDebug() << "Quit App!!!!!";
    if(routePointNum > 0)
    {
        mapMangr->save_route_points();
        mapMangr->save_route_waypoints();
    }
    fServer->stop_file_server();
    fServer->close_wifi();

    sysPwrOffPrcss->start("bash", QStringList() << "-c" << "sudo shutdown -h +0");

    delete mCanvas;
    delete trackCanvas;
    delete mMarker;
    delete trackMarker;
    mMarker = nullptr;
    trackMarker = nullptr;
    delete ui;
}


//ui->mapWidget->setMagnificationFactor(lupaVal);
//QTimer::singleShot(500, this, SLOT(gpioMangr->gpio_init()));




















