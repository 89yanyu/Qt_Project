#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QtEndian>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_serialport(new QSerialPort),
    m_serialprotocol(nullptr)
{
    ui->setupUi(this);
    isRunning = false;
    setWindowFlags(Qt::WindowCloseButtonHint);
    on_FreshSerialpushButton_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_serialport;
    delete m_serialprotocol;
}

void MainWindow::CloseSerialPort()
{
    if (m_serialport != nullptr)
    {
        m_serialport->close();
    }
    UnlockSerialList();
    ui->OpenSerialpushButton->setText(QString::fromLocal8Bit("打开串口"));
}

void MainWindow::LockSerialList()
{
    ui->SerialPortcomboBox->setEnabled(false);
    ui->FreshSerialpushButton->setEnabled(false);
}

void MainWindow::UnlockSerialList()
{
    ui->SerialPortcomboBox->setEnabled(true);
    ui->FreshSerialpushButton->setEnabled(true);
}

void MainWindow::on_OpenSerialpushButton_clicked()
{
    if (ui->OpenSerialpushButton->text() == QString::fromLocal8Bit("打开串口"))
    {
        LockSerialList();
        isRunning = true;
        m_serialport->setPortName(ui->SerialPortcomboBox->currentText());
        m_serialport->setBaudRate(QSerialPort::Baud115200);
        m_serialport->setStopBits(QSerialPort::OneStop);
        m_serialport->setDataBits(QSerialPort::Data8);
        m_serialport->setParity(QSerialPort::NoParity);
        m_serialport->setFlowControl(QSerialPort::NoFlowControl);
        if (!m_serialport->open(QIODevice::ReadWrite))
        {
            QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("串口打开失败！"));
            qDebug("Serial port open failed.");
            UnlockSerialList();
            return;
        }
        if (m_serialprotocol != nullptr)
        {
            delete m_serialprotocol;
        }
        m_serialprotocol = new SerialProtocol(m_serialport);
        connect(m_serialport, SIGNAL(readyRead()), m_serialprotocol, SLOT(procReadyRead()));
        connect(m_serialprotocol, SIGNAL(pushNewMsg(int,QByteArray)),
                this, SLOT(procNewMsg(int, QByteArray)));
        ui->OpenSerialpushButton->setText(QString::fromLocal8Bit("关闭串口"));
        LastKp = 1e20;
        LastKi = 1e20;
        LastKd = 1e20;
        LastTargetSpeed = 1e20;
        qDebug("Serial port open succeed.");
    }
    else if (ui->OpenSerialpushButton->text() == QString::fromLocal8Bit("关闭串口"))
    {
        isRunning = false;
        if (m_serialport->isOpen())
        {
            m_serialport->close();
            qDebug("Serial port close succeed.");
        }
        else
        {
            qDebug("Serial port is closed.");
        }
        UnlockSerialList();
        ui->OpenSerialpushButton->setText(QString::fromLocal8Bit("打开串口"));
    }
}

void MainWindow::on_FreshSerialpushButton_clicked()
{
    QList<QSerialPortInfo> PortList = QSerialPortInfo::availablePorts();
    ui->SerialPortcomboBox->clear();
    foreach (auto i, PortList)
    {
        ui->SerialPortcomboBox->addItem(i.portName());
    }
}

void MainWindow::LockControl()
{
    ui->KplineEdit->setEnabled(false);
    ui->KilineEdit->setEnabled(false);
    ui->KdlineEdit->setEnabled(false);

    ui->OpenSerialpushButton->setEnabled(false);
    ui->SaveDatapushButton->setEnabled(false);
}

void MainWindow::UnlockControl()
{
    ui->KplineEdit->setEnabled(true);
    ui->KilineEdit->setEnabled(true);
    ui->KdlineEdit->setEnabled(true);

    ui->OpenSerialpushButton->setEnabled(true);
    ui->SaveDatapushButton->setEnabled(true);
}

void MainWindow::on_StartTestpushButton_clicked()
{
    QByteArray tmp, out;
    char temp[8];
    if (!m_serialport->isOpen())
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("串口未连接！"));
        qDebug("Test start failed, serial is unconnected.");

        return;
    }

    ui->StartTestpushButton->setEnabled(false);
    ui->StopTestpushButton->setEnabled(false);
    ui->TargetSpeedlineEdit->setEnabled(false);
    if (ui->StartTestpushButton->text() == QString::fromLocal8Bit("启动测试"))
    {
        LockControl();
        m_saveState.clear();
        bool Kpok = false, Kiok = false, Kdok = false, TargetSpeedok = false;
        double Kp = ui->KplineEdit->text().toDouble(&Kpok);
        double Ki = ui->KilineEdit->text().toDouble(&Kiok);
        double Kd = ui->KdlineEdit->text().toDouble(&Kdok);
        double TargetSpeed = ui->TargetSpeedlineEdit->text().toDouble(&TargetSpeedok);
        if ((!Kpok) || (!Kiok) || (!Kdok) || (!TargetSpeedok))
        {
            QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("控制参数非法！"));
            qDebug("Set control value is not float.");
            ui->StartTestpushButton->setEnabled(true);
            ui->TargetSpeedlineEdit->setEnabled(true);
            UnlockControl();
            return;
        }

        if ((Kp != LastKp) || (Ki != LastKi) || (Kd != LastKd))
        {
            int cnt = 0;
            do
            {
                qDebug("Change PID");
                tmp.clear();
                qToBigEndian(Kp, temp);
                tmp.append(QByteArray::fromRawData(temp, sizeof(double)));
                qToBigEndian(Ki, temp);
                tmp.append(QByteArray::fromRawData(temp, sizeof(double)));
                qToBigEndian(Kd, temp);
                tmp.append(QByteArray::fromRawData(temp, sizeof(double)));
                SerialProtocol::Encode(SerialProtocol::USART_CONTROL_SETPID, 3 * sizeof(double), tmp, out);
                response = SerialProtocol::USART_CONTROL_MAX;
                m_serialport->write(out);
                m_serialport->flush();
                int start = QDateTime::currentDateTime().toTime_t();
                while (response == SerialProtocol::USART_CONTROL_MAX)
                {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                    if (QDateTime::currentDateTime().toTime_t() - start > 1)
                    {
                        break;
                    }
                }
                cnt++;
            }
            while ((response == SerialProtocol::USART_CONTROL_MAX) && (cnt < 3));
            if (response != SerialProtocol::USART_CONTROL_ACK)
            {
                QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("设置PID参数失败！"));
                if (cnt >= 3)
                {
                    CloseSerialPort();
                }
                ui->StartTestpushButton->setEnabled(true);
                ui->TargetSpeedlineEdit->setEnabled(true);
                UnlockControl();
                return;
            }
        }
        if (TargetSpeed != LastTargetSpeed)
        {
            int cnt = 0;
            do
            {
                qDebug("Change target speed");
                tmp.clear();
                qToBigEndian(TargetSpeed, temp);
                tmp.append(QByteArray::fromRawData(temp, sizeof(double)));
                SerialProtocol::Encode(SerialProtocol::USART_CONTROL_SETSPEED, sizeof(double), tmp, out);
                response = SerialProtocol::USART_CONTROL_MAX;
                m_serialport->write(out);
                m_serialport->flush();
                int start = QDateTime::currentDateTime().toTime_t();
                while (response == SerialProtocol::USART_CONTROL_MAX)
                {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                    if (QDateTime::currentDateTime().toTime_t() - start > 1)
                    {
                        break;
                    }
                }
                cnt++;
            }
            while ((response == SerialProtocol::USART_CONTROL_MAX) && (cnt < 3));
            if (response != SerialProtocol::USART_CONTROL_ACK)
            {
                QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("设置目标转速失败！"));
                if (cnt >= 3)
                {
                    CloseSerialPort();
                }
                ui->StartTestpushButton->setEnabled(true);
                ui->TargetSpeedlineEdit->setEnabled(true);
                UnlockControl();
                return;
            }

            ui->Graphwidget->setTargetSpeed(TargetSpeed);
        }

        int cnt = 0;
        do
        {
            tmp.clear();
            SerialProtocol::Encode(SerialProtocol::USART_CONTROL_START, 0, tmp, out);
            response = SerialProtocol::USART_CONTROL_MAX;
            m_serialport->write(out);
            m_serialport->flush();
            int start = QDateTime::currentDateTime().toTime_t();
            while (response == SerialProtocol::USART_CONTROL_MAX)
            {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                if (QDateTime::currentDateTime().toTime_t() - start > 1)
                {
                    break;
                }
            }
            cnt++;
        }
        while ((response == SerialProtocol::USART_CONTROL_MAX) && (cnt < 3));
        if (response != SerialProtocol::USART_CONTROL_ACK)
        {
            QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("启动失败"));
            if (cnt >= 3)
            {
                CloseSerialPort();
            }
            UnlockControl();
            ui->StartTestpushButton->setEnabled(true);
            ui->TargetSpeedlineEdit->setEnabled(true);
            return;
        }
        LastKp = Kp;
        LastKi = Ki;
        LastKd = Kd;
        LastTargetSpeed = TargetSpeed;
        ui->Graphwidget->Start();
        ui->StartTestpushButton->setText(QString::fromLocal8Bit("修改转速"));
    }
    else
    {
        bool TargetSpeedok = false;
        double TargetSpeed = ui->TargetSpeedlineEdit->text().toDouble(&TargetSpeedok);

        if (!TargetSpeedok)
        {
            QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("控制参数非法！"));
            qDebug("Set control value is not float.");
            ui->StartTestpushButton->setEnabled(true);
            ui->StopTestpushButton->setEnabled(true);
            ui->TargetSpeedlineEdit->setEnabled(true);
            return;
        }
        if (TargetSpeed != LastTargetSpeed)
        {
            int cnt = 0;
            do
            {
                qDebug("Change target speed");
                tmp.clear();
                qToBigEndian(TargetSpeed, temp);
                tmp.append(QByteArray::fromRawData(temp, sizeof(double)));
                SerialProtocol::Encode(SerialProtocol::USART_CONTROL_SETSPEED, sizeof(double), tmp, out);
                response = SerialProtocol::USART_CONTROL_MAX;
                m_serialport->write(out);
                m_serialport->flush();
                int start = QDateTime::currentDateTime().toTime_t();
                while (response == SerialProtocol::USART_CONTROL_MAX)
                {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                    if (QDateTime::currentDateTime().toTime_t() - start > 1)
                    {
                        break;
                    }
                }
                cnt++;
            }
            while ((response == SerialProtocol::USART_CONTROL_MAX) && (cnt < 3));
            if (response != SerialProtocol::USART_CONTROL_ACK)
            {
                QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("设置目标转速失败！"));
                if (cnt >= 3)
                {
                    CloseSerialPort();
                }
                UnlockControl();
                if (ui->StartTestpushButton->text() == QString::fromLocal8Bit("修改转速"))
                {
                    ui->StartTestpushButton->setText(QString::fromLocal8Bit("启动测试"));
                }
                ui->StartTestpushButton->setEnabled(true);
                ui->StopTestpushButton->setEnabled(false);
                ui->TargetSpeedlineEdit->setEnabled(true);
                return;
            }

            ui->Graphwidget->setTargetSpeed(TargetSpeed);
        }
        LastTargetSpeed = TargetSpeed;
    }
    ui->StartTestpushButton->setEnabled(true);
    ui->StopTestpushButton->setEnabled(true);
    ui->TargetSpeedlineEdit->setEnabled(true);
}

void MainWindow::on_StopTestpushButton_clicked()
{
    QByteArray tmp, out;
    ui->StopTestpushButton->setEnabled(false);
    ui->StartTestpushButton->setEnabled(false);
    ui->TargetSpeedlineEdit->setEnabled(false);
    if (!m_serialport->isOpen())
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("串口未连接！"));
        qDebug("Test start failed, serial is unconnected.");

        if (ui->StartTestpushButton->text() == QString::fromLocal8Bit("修改转速"))
        {
            ui->StartTestpushButton->setText(QString::fromLocal8Bit("启动测试"));
        }
        ui->StartTestpushButton->setEnabled(true);
        ui->StopTestpushButton->setEnabled(false);
        ui->TargetSpeedlineEdit->setEnabled(false);
        return;
    }
    int cnt = 0;
    do
    {
        tmp.clear();
        SerialProtocol::Encode(SerialProtocol::USART_CONTROL_STOP, 0, tmp, out);
        response = SerialProtocol::USART_CONTROL_MAX;
        m_serialport->write(out);
        m_serialport->flush();
        int start = QDateTime::currentDateTime().toTime_t();
        while (response == SerialProtocol::USART_CONTROL_MAX)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            if (QDateTime::currentDateTime().toTime_t() - start > 3)
            {
                break;
            }
        }
        cnt++;
    }
    while ((response == SerialProtocol::USART_CONTROL_MAX) && (cnt < 3));
    if (cnt >= 3)
    {
        CloseSerialPort();
    }
    ui->Graphwidget->Stop();
    UnlockControl();
    ui->StartTestpushButton->setText(QString::fromLocal8Bit("启动测试"));
    ui->StartTestpushButton->setEnabled(true);
    ui->TargetSpeedlineEdit->setEnabled(true);
}

void MainWindow::procNewMsg(int t, QByteArray msg)
{
    SerialProtocol::USART_CONTROL_TYP op = (SerialProtocol::USART_CONTROL_TYP)t;
    if (op == SerialProtocol::USART_CONTROL_ACK)
    {
        response = SerialProtocol::USART_CONTROL_ACK;
        qDebug("ACK");
    }
    if (op == SerialProtocol::USART_CONTROL_DEBUG)
    {
        qDebug("Debug");
        qDebug(msg);
    }
    if (op == SerialProtocol::USART_CONTROL_NAK)
    {
        response = SerialProtocol::USART_CONTROL_NAK;
        qDebug("NAK");
    }
    if (op == SerialProtocol::USART_CONTROL_SETPID) qDebug("Set PID");
    if (op == SerialProtocol::USART_CONTROL_SETSPEED) qDebug("Set speed");
    if (op == SerialProtocol::USART_CONTROL_START) qDebug("Start");
    if (op == SerialProtocol::USART_CONTROL_STATE)
    {
        short control, speed;
        control = (short)(((msg[0] & 0xff) << 8) | (msg[1] & 0xff));
        speed = (short)(((msg[2] & 0xff) << 8) | (msg[3] & 0xff));
        ui->NowControllineEdit->setText(QString::asprintf("%d", control));
        ui->NowSpeedlineEdit->setText(QString::asprintf("%d", speed));
        ui->Graphwidget->AddPoint(speed);
        if (isRunning) m_saveState.append(qMakePair(control, speed));
        //qDebug("State");
        //qDebug(qPrintable(QString::asprintf("%d %d\n", control, speed)));
    }
    if (op == SerialProtocol::USART_CONTROL_STOP) qDebug("Stop");
    //qDebug(msg.toHex());
}


void MainWindow::on_SaveDatapushButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("保存数据"),
                                                          QString::asprintf("./PIDData-%g-%g-%g-%s",
                                                                            LastKp, LastKi, LastKd,
                                                                            qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz"))),
                                                          "*.txt"); //选择路径
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
    foreach (auto x, m_saveState)
    {
        out << x.first << "," << x.second << endl;
    }
    file.close();
}
