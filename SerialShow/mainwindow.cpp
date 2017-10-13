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
    m_serialport(new QSerialPort)
{
    ui->setupUi(this);
    m_serialstream.setDevice(m_serialport);
    UpdateComboList();
    LastKp = LastKi = LastKd = HUGE_VAL;
}

void MainWindow::UpdateComboList()
{
    QList<QSerialPortInfo> PortList = QSerialPortInfo::availablePorts();
    ui->SerialNameComboBox->clear();
    foreach (auto i, PortList)
    {
        ui->SerialNameComboBox->addItem(i.portName());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_serialport;
}

void MainWindow::on_RefreshButton_clicked()
{
    UpdateComboList();
}

void MainWindow::CloseSerialPort()
{
    if (m_serialport->isOpen())
    {
        m_serialport->close();
        qDebug("Serial port close succeed.");
    }
    ui->OpenButton->setText("打开端口");
    ui->BaudRatComboBox->setEnabled(true);
    ui->StopBitsComboBox->setEnabled(true);
    ui->WordLengthComboBox->setEnabled(true);
    ui->ParityComboBox->setEnabled(true);
    ui->SerialNameComboBox->setEnabled(true);
    ui->RefreshButton->setEnabled(true);
}

void MainWindow::on_OpenButton_clicked()
{
    if (ui->OpenButton->text() == "打开端口")
    {
        m_serialport->setPortName(ui->SerialNameComboBox->currentText());
        if (!m_serialport->open(QIODevice::ReadWrite))
        {
            QMessageBox::critical(this, "错误", "串口打开失败");
            qDebug("Serial port open failed.");
            return;
        }

        ui->BaudRatComboBox->setEnabled(false);
        ui->StopBitsComboBox->setEnabled(false);
        ui->WordLengthComboBox->setEnabled(false);
        ui->ParityComboBox->setEnabled(false);
        ui->SerialNameComboBox->setEnabled(false);
        ui->RefreshButton->setEnabled(false);

        //m_serialport->setBaudRate(QSerialPort::Baud115200);
        if (ui->BaudRatComboBox->currentIndex() == 0) m_serialport->setBaudRate(QSerialPort::Baud1200);
        else if (ui->BaudRatComboBox->currentIndex() == 1) m_serialport->setBaudRate(QSerialPort::Baud2400);
        else if (ui->BaudRatComboBox->currentIndex() == 2) m_serialport->setBaudRate(QSerialPort::Baud4800);
        else if (ui->BaudRatComboBox->currentIndex() == 3) m_serialport->setBaudRate(QSerialPort::Baud9600);
        else if (ui->BaudRatComboBox->currentIndex() == 4) m_serialport->setBaudRate(QSerialPort::Baud19200);
        else if (ui->BaudRatComboBox->currentIndex() == 5) m_serialport->setBaudRate(QSerialPort::Baud38400);
        else if (ui->BaudRatComboBox->currentIndex() == 6) m_serialport->setBaudRate(QSerialPort::Baud57600);
        else m_serialport->setBaudRate(QSerialPort::Baud115200);

        //m_serialport->setStopBits(QSerialPort::OneStop);
        if (ui->StopBitsComboBox->currentIndex() == 1) m_serialport->setStopBits(QSerialPort::OneAndHalfStop);
        else if (ui->StopBitsComboBox->currentIndex() == 2) m_serialport->setStopBits(QSerialPort::TwoStop);
        else m_serialport->setStopBits(QSerialPort::OneStop);

        //m_serialport->setDataBits(QSerialPort::Data8);
        if (ui->WordLengthComboBox->currentIndex() == 0) m_serialport->setDataBits(QSerialPort::Data5);
        else if (ui->WordLengthComboBox->currentIndex() == 1) m_serialport->setDataBits(QSerialPort::Data6);
        else if (ui->WordLengthComboBox->currentIndex() == 2) m_serialport->setDataBits(QSerialPort::Data7);
        else m_serialport->setDataBits(QSerialPort::Data8);

        //m_serialport->setParity(QSerialPort::NoParity);
        if (ui->ParityComboBox->currentIndex() == 1) m_serialport->setParity(QSerialPort::OddParity);
        else if (ui->ParityComboBox->currentIndex() == 2) m_serialport->setParity(QSerialPort::EvenParity);
        else m_serialport->setParity(QSerialPort::NoParity);

        m_serialport->setFlowControl(QSerialPort::NoFlowControl);
        ui->OpenButton->setText("关闭端口");
        qDebug("Serial port open succeed.");
    }
    else if (ui->OpenButton->text() == "关闭端口")
    {
        CloseSerialPort();
    }
}

void MainWindow::waitmillsecond(int millsecond)
{
    QTime dieTime = QTime::currentTime().addMSecs(millsecond);
    while(QTime::currentTime() < dieTime)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

void MainWindow::on_SendButton_clicked()
{
    if (!m_serialport->isOpen())
    {
        QMessageBox::critical(this, "错误", "串口未打开");
        qDebug("Test start failed, serial port is closed.");
        return;
    }

    QString Line;
    ui->SendButton->setEnabled(false);

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
        m_serialstream << QString::asprintf("SETPID %g %g %g\r\n", Kp, Ki, Kd);
        m_serialstream.flush();

        waitmillsecond(100);
        m_serialstream.readLineInto(&Line);

        QStringList SplRes = Line.split(" = ");
        if ((SplRes.size() != 4) ||
            (SplRes.at(1).split(",").at(0).toDouble() != Kp) ||
            (SplRes.at(2).split(",").at(0).toDouble() != Ki) ||
            (SplRes.at(3).split(",").at(0).toDouble() != Kd))
        {
            QMessageBox::critical(this, "错误", "PID设置失败");
            qDebug("Set PID failed, something wrong with PLC.");
            ui->SendButton->setEnabled(true);
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

    m_serialstream << "START\r\n";
    m_serialstream.flush();
    if (m_serialstream.status() != QTextStream::Ok)
    {
        CloseSerialPort();
        qDebug("Test start failed, something wrong with serial port.");
    }
    qDebug("Test start succeed.");

    waitmillsecond(1200);
    qDebug("Finish");
    points.clear();
    while (m_serialstream.readLineInto(&Line))
    {
        if (Line.length() == 0) continue;
        points.push_back(Line.toInt());
    }
    ui->Graph->UpdatePoints(points);

    ui->SendButton->setEnabled(true);
}

void MainWindow::on_SaveButton_clicked()
{
    if (points.size() == 0) return;
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
    for (int i = 0; i < points.size(); i++)
    {
        out << points.at(i) << endl;
    }
    file.close();
}
