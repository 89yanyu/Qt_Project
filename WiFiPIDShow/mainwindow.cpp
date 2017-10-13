#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTime>
#include <QPainter>
#include <QFileDialog>
#include <QFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_TcpServer(new QTcpServer),
    m_TcpSocket(nullptr)
{
    qDebug("Start!");
    ui->setupUi(this);
    LastKp = LastKi = LastKd = HUGE_VAL;
    this->m_TcpServer->listen(QHostAddress::Any, 23333);
    qDebug("Listening at %d.", m_TcpServer->serverPort());
    connect(m_TcpServer, &QTcpServer::newConnection, this, &MainWindow::ProcConnection);
    ui->SendButton->setEnabled(false);
    ui->SaveButton->setEnabled(false);
}

void MainWindow::ProcConnection()
{
    if (nullptr != m_TcpSocket)
    {
        m_TcpSocket->abort();
    }
    m_TcpSocket = m_TcpServer->nextPendingConnection();
    qInfo("%s:%d connected.", m_TcpSocket->peerAddress(), m_TcpSocket->peerPort());
    m_wifistream.setDevice(m_TcpSocket);
    ui->SendButton->setEnabled(true);
    ui->SaveButton->setEnabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_TcpSocket != nullptr) delete m_TcpSocket;
    delete m_TcpServer;
}

void MainWindow::on_SendButton_clicked()
{
    if ((m_TcpSocket == nullptr) || (!m_TcpSocket->isOpen()))
    {
        QMessageBox::critical(this, "错误", "WiFi未连接");
        qDebug("Test start failed, wifi is unconnected.");
        return;
    }

    QString Line;
    ui->SendButton->setEnabled(false);
    ui->SaveButton->setEnabled(false);

    bool Kpok = false, Kiok = false, Kdok = false;
    double Kp = ui->KpLineEdit->text().toDouble(&Kpok);
    double Ki = ui->KiLineEdit->text().toDouble(&Kiok);
    double Kd = ui->KdLineEdit->text().toDouble(&Kdok);
    if ((!Kpok) || (!Kiok) || (!Kdok))
    {
        QMessageBox::critical(this, "错误", "PID值不合法");
        qDebug("Set PID failed, Input value is not float.");
        ui->SendButton->setEnabled(true);
        return;
    }

    if ((Kp != LastKp) || (Ki != LastKi) || (Kd != LastKd))
    {
        m_wifistream << QString::asprintf("SETPID %g %g %g\r\n", Kp, Ki, Kd);
        m_wifistream.flush();

        if (!m_TcpSocket->waitForReadyRead())
        {
            if (m_TcpSocket->isOpen())
            {
                m_TcpSocket->close();
                qDebug("wifi close succeed.");
            }
            qDebug("Set PID failed, something wrong with socket.");
            ui->SendButton->setEnabled(true);
            ui->SaveButton->setEnabled(true);
            return;
        }
        Line = "";
        int tt = 3;
        while (Line.length() == 0)
        {
            m_wifistream.readLineInto(&Line);
            tt--;
            if (tt == 0)
            {
                break;
            }
        }

        QStringList SplRes = Line.split(" = ");
        if ((SplRes.size() != 4) ||
            (SplRes.at(1).split(",").at(0).toDouble() != Kp) ||
            (SplRes.at(2).split(",").at(0).toDouble() != Ki) ||
            (SplRes.at(3).split(",").at(0).toDouble() != Kd))
        {
            QMessageBox::critical(this, "错误", "PID设置失败");
            qDebug("Set PID failed, something wrong with PLC. String: %s", qPrintable(Line));
            ui->SendButton->setEnabled(true);
            ui->SaveButton->setEnabled(true);
            return;
        }
        qDebug("Set PID succeed.");
        LastKp = Kp;
        LastKi = Ki;
        LastKd = Kd;
    }
    else
    {
        qDebug("PID unchange.");
    }

    m_wifistream << "START\r\n";
    m_wifistream.flush();

    if (!m_TcpSocket->waitForReadyRead())
    {
        if (m_TcpSocket->isOpen())
        {
            m_TcpSocket->close();
            qDebug("wifi close succeed.");
        }
        qDebug("Set PID failed, something wrong with socket.");
        ui->SendButton->setEnabled(true);
        ui->SaveButton->setEnabled(true);
        return;
    }
    qDebug("Finish");
    for (int i = 0; i < pointstypecnt; i++)
    {
        points[i].clear();
    }
    for (int i = 1; i <= 100; i++)
    {
        for (int j = 0; j < pointstypecnt; j++)
        {
            int tmp;
            m_wifistream >> tmp;
            points[j].push_back(tmp);
        }
    }
    ui->Graph->UpdatePoints(points, pointstypecnt);

    ui->SendButton->setEnabled(true);
    ui->SaveButton->setEnabled(true);
}

void MainWindow::on_SaveButton_clicked()
{
    if (points[0].size() == 0) return;
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("保存数据"),
                                                    QString::asprintf("./PIDData-%g-%g-%g-%s",
                                                                      LastKp, LastKi, LastKd,
                                                                      qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz"))),
                                                    tr("*.txt")); //选择路径
    if(filename.isEmpty())
    {
        return;
    }

    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "错误", "数据保存失败");
        qDebug("Can't open the file!");
        return;
    }
    QTextStream out(&file);
    for (int i = 0; i < points[0].size(); i++)
    {
        out << points[0].at(i) << endl;
    }
    file.close();
}
