#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "procthread.h"

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
    void on_horizontalSlider_valueChanged(int value);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void procCompleteTack(int row, int state);
    void closeEvent(QCloseEvent *event);

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    ProcThread *t;

signals:
    void newTask(QString, int);
};

#endif // MAINWINDOW_H
