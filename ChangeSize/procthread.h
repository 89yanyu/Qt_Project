#ifndef PROCTHREAD_H
#define PROCTHREAD_H

#include <QQueue>
#include <QPair>
#include <QString>
#include <QThread>

#define ScaledIMG(img, x, Path, y) (img).scaled(QSize(qRound((img).width() * (x)), qRound((img).height() * (x))), Qt::IgnoreAspectRatio, Qt::FastTransformation).save((Path), "JPG", (y))

class ProcThread : public QThread
{
    Q_OBJECT
public:
    ProcThread();
    void stop();
    void setAndStart(int s);
protected:
    void run();
private:
    volatile bool stopped;
    int size;
    QString suffix;
    QQueue<QPair<QString, int> > taskLine;
signals:
    void completeTask(int, int);
public slots:
    void addTask(QString file, int row);
};

#endif // PROCTHREAD_H
