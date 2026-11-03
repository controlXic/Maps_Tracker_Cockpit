#ifndef SWITCH_H
#define SWITCH_H

#include <QCheckBox>
#include <QPainter>
#include <QPropertyAnimation>

class Switch : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(int circle_pos READ circle_pos WRITE set_circle_pos) // Para animación
public:
    explicit Switch(QWidget *parent = nullptr);

    int circle_pos() const { return m_circlePos; }

    void set_circle_pos(int pos);

protected:

    void paintEvent(QPaintEvent *) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_circlePos = 3;
    QPropertyAnimation *m_animation;
};

#endif // SWITCH_H
