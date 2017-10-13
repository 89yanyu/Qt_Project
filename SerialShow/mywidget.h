#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>

class MyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MyWidget(QWidget *parent = 0);

    ~MyWidget();

    void UpdatePoints(QList<int> points);

    void paintEvent(QPaintEvent *event);

    void resizeEvent(QResizeEvent *event);

private:
    QPixmap *pix;

signals:

public slots:
};

#endif // MYWIDGET_H
