#ifndef GPIO_MANAGER_H
#define GPIO_MANAGER_H

#include <QObject>

class gpio_manager : public QObject
{
    Q_OBJECT
public:
    explicit gpio_manager(QObject *parent = nullptr);


    bool gpio_init();

    uint8_t gpio_read();

signals:

};

#endif // GPIO_MANAGER_H
