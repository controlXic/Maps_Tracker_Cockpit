#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtCharts>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>

#include <qgsvertexmarker.h>
#include <qgsmapcanvas.h>
#include <qgsvectortilelayer.h>
#include <qgsmaptoolpan.h>

#include "map_manager.h"
#include "serialport_manager.h"
#include "config_manager.h"
//#include "i2c_manager.h"
#include "file_server.h"
#include "gauge.h"
#include "gpio_manager.h"

class chart_manager;


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void on_centerBtn_clicked();

    void on_zoomInBtn_clicked();

    void on_zoomOutBtn_clicked();

    void on_gpxOpenBtn_clicked();

    void on_startTestBtn_clicked();

    void on_stopTestBtn_clicked();

    void on_gpxQuitBtn_clicked();

    void on_saveRouteBtn_clicked();

    void update_app_status();

    void load_vector_map();

    void simulate_route_points();

    void set_gps_position(float lon, float lat);

    void set_track_position(bool icon, float lon, float lat);

    void open_gpx(QString gpxFile);

    void set_elevation_graphic();

    void get_gpx_files();

    void on_tabWidget_currentChanged(int index);

    void toggle_view_places(bool status);

    void toggle_view_fuels(bool status);

    void toggle_view_infoLabel(bool status);

    void toggle_view_nightMode(bool status);

    void toggle_view_wifi(bool status);

    void toggle_view_fullScrn(bool status);

    void toggle_view_kl30(bool status);

    void clear_elevation_graphic();

    double map_rotation(float lat1, float lon1, float lat2, float lon2);

    void set_route_layer();

    void add_point_route(float lat, float lon, float ele);

    void add_waypoint(float lat, float lon, float ele);

    void on_resetRouteBtn_clicked();

    void reset_route();

    void on_waypointBtn_clicked();

    void app_update_callback();

    void on_cntRotationBtn_clicked();

    void on_cntCatBtn_clicked();

    void on_cntHomeBtn_clicked();

    void on_quitBtn_clicked();

    void center_in_coord(float lat, float lon);

    void on_clrGpsBtn_clicked();

    void on_rmMarkerBtn_clicked();

    void on_rstKmBtn_clicked();

    void system_pwr_off();

    void on_pwerOffBtn_clicked();

private:
    // map vars //
    QgsMapCanvas *mCanvas;
    QgsMapCanvas *trackCanvas;
    QgsMapToolPan  *panTool;
    QgsVertexMarker *mMarker;
    QgsVertexMarker *trackMarker;

    // grafic vars //
    QGraphicsLineItem *m_line;
    QGraphicsSimpleTextItem *m_text;
    QChart *chartG;
    QChartView *chartView;

    // Declaració d'objectes //
    serialport_manager *spMangr = nullptr;
    map_manager *mapMangr = nullptr;
//    i2c_manager *i2cMangr = nullptr;
    file_server *fServer = nullptr;
    config_manager *configMangr = nullptr;
    gpio_manager *gpioMangr = nullptr;

    // struct map_manager //
    GpsPoint gpsPnt;
    initRouteVars initRouteV;

    Ui::MainWindow *ui;
};

/////////////////////////////////////////////
// Global vars
/////////////////////////////////////////////

// GPS vars //
extern float gpsPoint[3]; //0=Lat; 1=Lon; 2=Ele;
extern bool gpsReadFlag;

// track vars //
extern float trackPoint[4][50000]; //0=Lat; 1=Lon; 2=Ele; 3=Km
extern float maxElevation;
extern float kmTotal;
extern uint16_t trackPointNum;

// route vars //
extern float routePoint[3][50000]; //[0=Lon; 1=Lat; 2=Ele][routePointCnt];
extern QStringList routeDateHour;
extern uint16_t routePointCnt;
extern uint16_t routePointNum;

// waypoint vars //
extern float waypoint[3][500]; // 0=Lon; 1=Lat; 2=Ele
extern QStringList waypointDateHour;
extern uint16_t waypointCnt;
extern uint16_t waypointNum;


#endif // MAINWINDOW_H
