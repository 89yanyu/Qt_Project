#include "procthread.h"
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QBuffer>

ProcThread::ProcThread()
{
    this->stopped = false;
}

void ProcThread::stop()
{
    this->stopped = true;
}

void ProcThread::setAndStart(int s)
{
    this->size = s;
    if (s < 1000000)
    {
        this->suffix = QString::asprintf("%dKB", s / 1000);
    }
    else
    {
        this->suffix = QString::asprintf("%dMB", s / 1000000);
    }
    this->start();
}

void ProcThread::run()
{
    qDebug(QString::asprintf("%.2f", this->size / 1024.).toLocal8Bit());
    while (!this->stopped)
    {
        if (taskLine.isEmpty())
        {
            this->msleep(1);
            continue;
        }
        QString file = taskLine.first().first;
        int row = taskLine.first().second;
        taskLine.pop_front();

        QImage img;
        if (!img.load(file))
        {
            emit completeTask(row, 1);
            continue;
        }
        double l = 0, r = 1., mid = 0;
        int n = 10;
        QString name = file.right(file.length() - file.lastIndexOf('/') - 1);
        name = name.left(name.lastIndexOf('.')) + " - " + this->suffix + ".jpg";
        while (n--)
        {
            mid = (l + r) / 2.;
            ScaledIMG(img, mid, name, qRound(mid * mid * 100));
            QFileInfo fi(name);
            qDebug(QString::asprintf("%.5f %.2f", mid, fi.size() / 1024.).toLocal8Bit());
            if (fi.size() < this->size)
            {
                l = mid;
            }
            else
            {
                r = mid;
            }
        }
        ScaledIMG(img, l, name, qRound(mid * mid * 100));
        emit completeTask(row, 0);
    }
    qDebug("thread out");
}

void ProcThread::addTask(QString file, int row)
{
    taskLine.push_back(QPair<QString, int>(file, row));
}
