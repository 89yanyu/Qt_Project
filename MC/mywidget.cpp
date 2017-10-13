#include "mywidget.h"
#include <QPainter>

MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent)
{
    pix = new QPixmap(this->size());
    pix->fill(Qt::black);
    isRunning = false;
}

MyWidget::~MyWidget()
{
    delete pix;
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
        QPixmap *newPix = new QPixmap(this->size());
        newPix->fill(Qt::black);
        pix = newPix;
    }
    QWidget::resizeEvent(event);
}

void MyWidget::Stop()
{
    isRunning = false;
    paint(SavePoints, firstTarget);
}

void MyWidget::Start()
{
    SavePoints.clear();
    Points.clear();
    isRunning = true;
    firstTarget = TargetSpeed;
}

void MyWidget::setTargetSpeed(double ts)
{
    TargetSpeed = ts;
}

#define changeX(x) (x * stepX)
#define changeY(x) ((MaxSpeed - x) * stepY)

void MyWidget::paint(QList<int> points, double speed)
{
    double stepX = pix->width() * 1. / points.length();
    double stepY = pix->height() * 1. / (2 * MaxSpeed);

    pix->fill(Qt::black);

    QPainter painter;

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(2);

    QPointF now, last;

    painter.begin(pix);

    pen.setColor(QColor(0, 255, 0));
    painter.setPen(pen);
    painter.drawLine(QPointF(0, changeY(speed)), QPointF(pix->width() - 1, changeY(speed)));

    pen.setColor(QColor(0, 85, 255));
    painter.setPen(pen);
    last.setX(changeX(0));
    last.setY(changeY(0));
    for(int i = 0; i < points.length(); i++)
    {
        now.setX(changeX(i + 1));
        now.setY(changeY(points[i]));
        painter.drawLine(last, now);
        last = now;
    }

    painter.end();

    update();
}

void MyWidget::AddPoint(int x)
{
    if (!isRunning) return;

    if (SavePoints.length() < MaxSaveCount)
    {
        SavePoints.push_back(x);
    }
    Points.push_back(x);
    if (Points.length() > MaxSaveCount)
    {
        Points.pop_front();
    }

    paint(Points, TargetSpeed);
}
