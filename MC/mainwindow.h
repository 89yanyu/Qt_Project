#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QLinkedList>
#include <QPair>
#include "serialprotocol.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_OpenSerialpushButton_clicked();

    void on_FreshSerialpushButton_clicked();

    void on_StartTestpushButton_clicked();

    void procNewMsg(int op, QByteArray msg);

    void on_StopTestpushButton_clicked();

    void on_SaveDatapushButton_clicked();

private:
    SerialProtocol::USART_CONTROL_TYP response;
    bool isRunning;
    Ui::MainWindow *ui;
    QSerialPort *m_serialport;
    SerialProtocol *m_serialprotocol;
    QLinkedList<QPair<short, short> > m_saveState;
    double LastKp, LastKi, LastKd, LastTargetSpeed;
    void LockControl();
    void UnlockControl();
    void LockSerialList();
    void UnlockSerialList();
    void CloseSerialPort();
};

#endif // MAINWINDOW_H
