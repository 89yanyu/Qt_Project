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

void MyWidget::UpdatePoints(QList<int> points[], int typecnt)
{
    const double maxspeed = 60.;
    const double trgspeed = 20.;
    pix->fill(Qt::black);

    QPainter painter;

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(2);

    QPointF now, last;
    double stepx = pix->width() * 1. / points[0].size();
    double stepy = pix->height() / maxspeed;

    painter.begin(pix);

    pen.setColor(QColor(0, 255, 0));
    painter.setPen(pen);
    painter.drawLine(QPointF(0, pix->height() - 1 - trgspeed * stepy), QPointF(pix->width() - 1, pix->height() - 1 - trgspeed * stepy));

    QColor linecolor[2] = {QColor(0, 85, 255),
                           QColor(255, 85, 255)};

    for (int j = 0; j < typecnt; j++)
    {
        double mean = 0.;
        pen.setColor(linecolor[j]);
        painter.setPen(pen);
        last.setX(0);
        last.setY(pix->height() - 1);
        for (int i = 0; i < points[j].size(); i++)
        {
            now.setX((i + 1) * stepx);
            now.setY(pix->height() - 1 - points[j][i] * stepy);
            if (i >= 10) mean += points[j][i];
            painter.drawLine(last, now);
            last = now;
        }
        qDebug("%g\n", mean / (points[j].size() - 10));
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
