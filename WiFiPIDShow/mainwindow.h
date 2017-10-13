#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTextStream>
#include <QTcpServer>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    static const int pointstypecnt = 2;
    double LastKp, LastKi, LastKd;
    QList<int> points[pointstypecnt];
    void ProcConnection();
    QTcpServer *m_TcpServer;
    QTcpSocket *m_TcpSocket;

private slots:
    void on_SendButton_clicked();

    void on_SaveButton_clicked();

private:
    Ui::MainWindow *ui;
    QTextStream m_wifistream;

};

#endif // MAINWINDOW_H
