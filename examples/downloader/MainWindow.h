#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class QFile;
class CurlEasy;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_startStopButton_clicked();
    void onTransferProgress(qint64 downloadTotal, qint64 downloadNow, qint64 uploadTotal, qint64 uploadNow);
    void onTransferDone();
    void onTransferAborted();

private:
    void log(QString text);

    Ui::MainWindow *ui;

    CurlEasy *transfer = nullptr;
    QFile *downloadFile = nullptr;
};

#endif // MAINWINDOW_H
