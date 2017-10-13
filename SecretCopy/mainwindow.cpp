#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileInfoList>
#include <QCryptographicHash>
#include <QDateTime>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    trayIcon(new QSystemTrayIcon(this)),
    timer(new QTimer(this)),
    cmd(new QProcess(0))
{
    setWindowFlags(Qt::WindowCloseButtonHint);
    ui->setupUi(this);
    trayIcon->setIcon(QIcon(":/ICON/red.ico"));
    trayIcon->show();
    trayIcon->setToolTip("");
    connect(timer,SIGNAL(timeout()),this,SLOT(scanDrives()));

    QFileInfoList initList = QDir::drives();
    foreach (QFileInfo dirver, initList)
    {
        ui->comboBox->addItem(dirver.absolutePath());
    }
    qDebug(QString::asprintf("All %d drives.", initList.length()).toLocal8Bit());

    extCopy.clear();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    if (ui->pptCheckBox->isChecked())
    {
        extCopy.append("ppt");
        extCopy.append("pptx");
    }
    if (ui->pdfCheckBox->isChecked())
    {
        extCopy.append("pdf");
    }
    if (ui->OtherheckBox->isChecked())
    {
        QString otherExt = ui->otherExtLineEdit->text();
        auto otherExtList = otherExt.split("|");
        foreach(QString ext, otherExtList)
        {
            if (ext.isEmpty())
            {
                continue;
            }
            if (!extCopy.contains(ext, Qt::CaseInsensitive))
            {
                extCopy.append(ext);
            }
        }
    }

    if (extCopy.size() == 0)
    {
        return;
    }

    QString allExt;
    for (int index = 0; index < extCopy.size(); index++)
    {
        if (index > 0)
        {
            allExt += "|";
        }
        allExt += extCopy.at(index);
    }

    if (QMessageBox::question(this, "Check ext", allExt, QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Cancel)
    {
        extCopy.clear();
        return;
    }

    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QByteArray bb = QCryptographicHash::hash(now.toLocal8Bit(), QCryptographicHash::Md5);
    QString md5 = bb.toHex();
    TargetDir = ui->comboBox->currentText() + md5 + "\\";
    TargetDir.replace('/', "\\");
    qDebug(TargetDir.toLocal8Bit());
    cmd->start("cmd");
    if (!cmd->waitForStarted(-1)) qDebug("Start failed.");
    cmd->write(("mkdir " + TargetDir + "\r\n").toLocal8Bit());
    if (!cmd->waitForReadyRead(-1)) qDebug("Make director failed.");
    cmd->write("exit\r\n");
    if (!cmd->waitForFinished(-1)) qDebug("Close failed.");

    timer->start(10000);
    this->hide();
}

void MainWindow::scanDrives()
{
    qDebug("Scanning...");
    QFileInfoList initList = QDir::drives();
    QString SourceDir = "";
    qDebug(QString::asprintf("Get %d drives.", initList.length()).toLocal8Bit());
    foreach (QFileInfo dirver, initList)
    {
        if (-1 == ui->comboBox->findText(dirver.absolutePath()))
        {
            SourceDir = dirver.absolutePath();
            break;
        }
    }
    if (SourceDir.length() == 0)
    {
        return;
    }

    qDebug(("Find " + SourceDir).toLocal8Bit());
    timer->stop();
    trayIcon->setIcon(QIcon(":/ICON/yellow.ico"));
    int start = QDateTime::currentDateTime().toTime_t();
    while (QDateTime::currentDateTime().toTime_t() - start <= 5)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }

    qDebug("Copying...");

    foreach(QString ext, extCopy)
    {
        qDebug(ext.toLocal8Bit());
        cmd->start("robocopy", QStringList() << SourceDir << TargetDir << "*." + ext << "/s");
        if (!cmd->waitForStarted(-1))
        {
            qDebug("Start failed.");
            break;
        }
        if (!cmd->waitForFinished(-1))
        {
            qDebug("Close failed.");
            break;
        }
    }

    trayIcon->setIcon(QIcon(":/ICON/green.ico"));
    extCopy.clear();
    qDebug("OK");
}
