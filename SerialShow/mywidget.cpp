#include "mywidget.h"
#include <QPainter>
#include <QPixmap>

MyWidget::MyWidget(QWidget *parent) : QWidget(parent)
{
    pix = new QPixmap(this->size());
    pix->fill(Qt::black);
}

MyWidget::~MyWidget()
{
    delete pix;
}

void MyWidget::UpdatePoints(QList<int> points)
{
    pix->fill(Qt::black);

    QPainter painter;

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(2);

    QPointF now, last(0, pix->height() - 1);
    double stepx = pix->width() * 1. / points.size();
    double stepy = pix->height() / 80.;

    painter.begin(pix);

    pen.setColor(QColor(0, 255, 0));
    painter.setPen(pen);
    painter.drawLine(QPointF(0, pix->height() - 1 - 70. * stepy), QPointF(pix->width() - 1, pix->height() - 1 - 70. * stepy));

    pen.setColor(QColor(0, 63, 255));
    painter.setPen(pen);
    for (int i = 0; i < points.size(); i++)
    {
        now.setX((i + 1) * stepx);
        now.setY(pix->height() - 1 - points[i] * stepy);
        painter.drawLine(last, now);
        last = now;
    }
    painter.end();

    update();
}

void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(QPoint(0,0), *pix);
}

void MyWidget::resizeEvent(QResizeEvent *event)
{
    if(height() > pix->height() || width() > pix->width())
    {
        QPixmap *newPix = new QPixmap(size());
        newPix->fill(Qt::black);
        QPainter p(newPix);
        pix = newPix;
    }
    QWidget::resizeEvent(event);
}
