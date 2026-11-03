#include "gauge.h"

#include <QPainter>
#include <QConicalGradient>
#include <QtMath>

gauge::gauge(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
}

void gauge::setValue(float value)
{
    m_value = qBound(0.0f, value, 160.0f);
    update();
}

void gauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF rect(gaugeWidth/2, gaugeWidth/2, width() - gaugeWidth, height() - gaugeWidth);

    // Arc base (gris) //
    QPen basePen(Qt::gray, gaugeWidth);
    basePen.setCapStyle(Qt::RoundCap);
    p.setPen(basePen);
    p.drawArc(rect, 315 * 16, 270 * 16);

    // Gradient (verd → blau) //
    QPointF center = rect.center();
    QConicalGradient gradient(center, -45);

    gradient.setColorAt(0.0, Qt::blue);
    gradient.setColorAt(0.5, Qt::cyan);
    gradient.setColorAt(1.0, Qt::green);

    QPen valuePen(QBrush(gradient), gaugeWidth);
    valuePen.setCapStyle(Qt::RoundCap);
    p.setPen(valuePen);

    // Valor convertit a angle //
    float spanAngle = (m_value / 160.0f) * 270.0f;

    p.drawArc(rect, 225 * 16, -spanAngle * 16);
}
