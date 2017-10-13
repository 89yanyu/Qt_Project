#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QList>

class MyWidget : public QWidget
{
public:
    MyWidget(QWidget *parent = 0);
    void Stop();
    void Start();
    void setTargetSpeed(double ts);
    void AddPoint(int x);
    ~MyWidget();
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *event);
private:
    QPixmap *pix;
    bool isRunning;
    QList<int> SavePoints;
    QList<int> Points;
    void paint(QList<int> points, double speed);
    const double MaxSpeed = 4500;
    const int MaxSaveCount = 600;
    double TargetSpeed;
    double firstTarget;
};

#endif // MYWIDGET_H
