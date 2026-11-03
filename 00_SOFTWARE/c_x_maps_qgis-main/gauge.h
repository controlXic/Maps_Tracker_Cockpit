#ifndef GAUGE_H
#define GAUGE_H

#include <QWidget>

class gauge : public QWidget
{
    Q_OBJECT
public:
    explicit gauge(QWidget *parent = nullptr);

    void setValue(float value); // 0 - 100

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_value = 0.0f;
    uint8_t gaugeWidth = 35;
};

#endif // GAUGE_H




