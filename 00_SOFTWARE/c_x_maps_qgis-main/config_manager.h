#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

//#include <QObject>
#include <QDebug>
#include <QStringList>
#include <QString>


class config_manager
{

public:
    explicit config_manager();

    void set_config_json();

    void write_config_json(QString objName, bool objStatus, QString fName);

    void read_config_json();

private:

};

// Track Tab //
extern QString trackName;
extern bool trackActive;
extern bool routeActive;

// View Tab //
extern bool placesActive;
extern bool fuelsActive;
extern bool trackInfoActive;
extern bool nightModeActive;
extern bool fullScreenActive;

// Cockpit Tab //
extern QString cntKms;

#endif // CONFIG_MANAGER_H
