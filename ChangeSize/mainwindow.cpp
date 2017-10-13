#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QList>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::WindowCloseButtonHint);
    this->setFixedSize(this->width(), this->height());
    ui->horizontalSlider->setAcceptDrops(false);
    ui->label->setAcceptDrops(false);
    ui->listWidget->setAcceptDrops(true);
    this->setAcceptDrops(true);
    t = new ProcThread();
    connect(this, SIGNAL(newTask(QString,int)), t, SLOT(addTask(QString,int)));
    connect(t, SIGNAL(completeTask(int,int)), this, SLOT(procCompleteTack(int,int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    t->stop();
    t->wait();
    event->accept();
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    QString text;
    QRect geo = ui->label->geometry();
    if (value < 10)
    {
        ui->label->setGeometry(165, geo.y(), geo.width(), geo.height());
        text = QString::asprintf("%dKB", value * 100);
    }
    else
    {
        ui->label->setGeometry(180, geo.y(), geo.width(), geo.height());
        text = QString::asprintf("%dMB", value - 9);
    }
    ui->label->setText(text);
}


void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasFormat("text/uri-list"))
    {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if(urls.isEmpty())
    {
        return;
    }

    foreach (QUrl u, urls) {
        QString file = u.toLocalFile();
        int pos = file.lastIndexOf('/');
        QString path = file.left(pos);
        QString name = file.right(file.length() - pos - 1);
        QString item = name + "(" + path + ")";
        bool isSame = false;
        for(int i = ui->listWidget->count() - 1; i >= 0; i--)
        {
            if (ui->listWidget->item(i)->text() == item)
            {
                isSame = true;
                break;
            }
        }
        if (!isSame)
        {
            emit newTask(file, ui->listWidget->count());
            ui->listWidget->addItem(item);
        }
    }
}

void MainWindow::procCompleteTack(int row, int state)
{
    QColor c(255, 0, 0);
    if (state == 0)
    {
        c.setRgb(0, 255, 0);
    }
    ui->listWidget->item(row)->setBackgroundColor(c);
}

void MainWindow::on_pushButton_clicked()
{
    static int first = 0;
    if (first == 0)
    {
        int size, value = ui->horizontalSlider->value();
        if (value < 10)
        {
            size = value * 100000;
        }
        else
        {
            size = (value - 9) * 1000000;
        }
        first = 1;
        ui->horizontalSlider->setEnabled(false);
        t->setAndStart(size);
        qDebug("first");
    }
}
