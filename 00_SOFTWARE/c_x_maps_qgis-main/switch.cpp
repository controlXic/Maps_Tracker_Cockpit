#include "switch.h"

#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QCheckBox>
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>

//int margin = 4;


Switch::Switch(QWidget *parent)
    : QCheckBox{parent}
{
    setFixedSize(50, 26);
    setCursor(Qt::PointingHandCursor);

    m_animation = new QPropertyAnimation(this, "circle_pos");
    m_animation->setDuration(150); // velocitat de desplaçament
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);

    // Animació del toggle
    connect(this, &QCheckBox::toggled, this, [this](bool checked){
        m_animation->stop();
        m_animation->setStartValue(m_circlePos);
        m_animation->setEndValue(checked ? (width() - height() + 2) : 3);
        m_animation->start();
    });
}

void Switch::set_circle_pos(int pos)
{
    m_circlePos = pos;
    update(); // repinta la boleta
}

void Switch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Fondo
    p.setBrush(isChecked() ? QColor("#3498db") : QColor("#ccc"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(0, 0, width(), height(), height()/2, height()/2);

    int circleSize = 20; // mida de la boleta
    int y = (height() - circleSize) / 2;
    p.setBrush(Qt::white);
    p.drawEllipse(m_circlePos, y, circleSize, circleSize);
}

void Switch::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->pos()))
    {
        setChecked(!isChecked());
    }
}

