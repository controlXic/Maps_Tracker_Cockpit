QT      += core gui
QT      += widgets
QT      += xml
QT      += charts
QT      += serialport
QT      += quick qml quickwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    config_manager.cpp \
    file_server.cpp \
    gauge.cpp \
    gpio_manager.cpp \
    i2c_manager.cpp \
    main.cpp \
    mainwindow.cpp \
    map_manager.cpp \
    serialport_manager.cpp \
    switch.cpp

HEADERS += \
    config_manager.h \
    file_server.h \
    gauge.h \
    gpio_manager.h \
    i2c_manager.h \
    mainwindow.h \
    map_manager.h \
    serialport_manager.h \
    switch.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


INCLUDEPATH += //usr/include/qgis
LIBS += -L/usr/lib/qgis -lqgis_core -lqgis_gui

INCLUDEPATH += //usr/include/gdal
LIBS += -lgdal

INCLUDEPATH += //usr/src/wiringOP/wiringPi
LIBS += -lwiringPi

DISTFILES +=

RESOURCES +=
