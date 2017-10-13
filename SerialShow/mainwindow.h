#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTextStream>

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
    void UpdateComboList();
    void CloseSerialPort();
    double LastKp, LastKi, LastKd;
    void waitmillsecond(int millsecond);
    QList<int> points;

private slots:
    void on_RefreshButton_clicked();

    void on_OpenButton_clicked();

    void on_SendButton_clicked();

    void on_SaveButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *m_serialport;
    QTextStream m_serialstream;
};

#endif // MAINWINDOW_H
