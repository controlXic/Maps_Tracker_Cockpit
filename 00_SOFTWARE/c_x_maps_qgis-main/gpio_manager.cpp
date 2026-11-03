#include "gpio_manager.h"

#include <QDebug>

#include <wiringPi.h>
#include <wiringPiI2C.h>

#define KL30 2
#define CLAUSOR 11


gpio_manager::gpio_manager(QObject *parent)
    : QObject{parent}
{
    wiringPiSetup();

    pinMode(2, INPUT); // KL30
    pinMode(11, INPUT); // clausor
}


bool gpio_manager::gpio_init()
{
    if(digitalRead(KL30) == HIGH && digitalRead(CLAUSOR) == HIGH)
    {
        qDebug() << "clausor = HIGH && KL30 = HIGH";

        return true;
    }
    else
    {
        qDebug() << "clausor = LOW && KL30 = LOW";

        return false;
    }
}

uint8_t gpio_manager::gpio_read()
{
    uint8_t res = 0;

    if(digitalRead(KL30) == LOW)
        res = 1;

    else if(digitalRead(KL30) == HIGH)
        res = 2;

    if(digitalRead(CLAUSOR) == LOW)
        res = 3;

    return res;
}
