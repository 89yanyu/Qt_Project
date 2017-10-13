#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QProcess>

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
    void on_pushButton_clicked();
    void scanDrives();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QTimer *timer;
    QString TargetDir;
    QProcess *cmd;
    QStringList extCopy;
};

#endif // MAINWINDOW_H
