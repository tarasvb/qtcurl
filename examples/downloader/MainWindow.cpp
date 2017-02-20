#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "CurlEasy.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create CurlEasy and connect signals.
    // Since curl_easy handles could be reused freely we can do it only once
    transfer = new CurlEasy(this); // Parent it so it will be destroyed automatically

    connect(transfer, &CurlEasy::done, this, &MainWindow::onTransferDone);
    connect(transfer, &CurlEasy::aborted, this, &MainWindow::onTransferAborted);
    connect(transfer, &CurlEasy::progress, this, &MainWindow::onTransferProgress);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startStopButton_clicked()
{
    // Are we "Abort" button?
    if (transfer->isRunning()) {
        transfer->abort();
        return;
    }

    ui->transferLog->clear();
    ui->progressBar->setValue(0);

    // Prepare target file
    downloadFile = new QFile(ui->fileNameEdit->text());
    if (!downloadFile->open(QIODevice::WriteOnly)) {
        log("Failed to open file for writing.");
        delete downloadFile;
        downloadFile = nullptr;
        return;
    }

    // Set a simple file writing function
    transfer->setWriteFunction([this](char *data, size_t size)->size_t {
        qint64 bytesWritten = downloadFile->write(data, static_cast<qint64>(size));
        return static_cast<size_t>(bytesWritten);
    });

    // Print headers to the transfer log box
    transfer->setHeaderFunction([this](char *data, size_t size)->size_t {
        log(QString::fromUtf8(data, static_cast<int>(size)));
        return size;
    });

    transfer->set(CURLOPT_URL, QUrl(ui->urlEdit->text()));
    transfer->set(CURLOPT_FOLLOWLOCATION, long(1)); // Follow redirects
    transfer->set(CURLOPT_FAILONERROR, long(1)); // Do not return CURL_OK in case valid server responses reporting errors.

    ui->startStopButton->setText("Abort");
    log("Transfer started.");

    transfer->perform();
}

void MainWindow::onTransferProgress(qint64 downloadTotal, qint64 downloadNow, qint64 uploadTotal, qint64 uploadNow)
{
    Q_UNUSED(uploadTotal);
    Q_UNUSED(uploadNow);

    if (downloadTotal > 0) {
        if (downloadNow > downloadTotal) downloadNow = downloadTotal;
        qint64 progress = (downloadNow * ui->progressBar->maximum())/downloadTotal;
        ui->progressBar->setValue(static_cast<int>(progress));
    } else {
        ui->progressBar->setValue(0);
    }
}

void MainWindow::onTransferDone()
{
    if (transfer->result() != CURLE_OK) {
        log(QString("Transfer failed with curl error '%1'")
                    .arg(curl_easy_strerror(transfer->result())));
        downloadFile->remove();
    } else {
        log(QString("Transfer complete. %1 bytes downloaded.")
                    .arg(downloadFile->size()));
        ui->progressBar->setValue(ui->progressBar->maximum());
    }

    delete downloadFile;
    downloadFile = nullptr;

    ui->startStopButton->setText("Start");
}

void MainWindow::onTransferAborted()
{
    log(QString("Transfer aborted. %1 bytes downloaded.")
        .arg(downloadFile->size()));

    downloadFile->remove();
    delete downloadFile;
    downloadFile = nullptr;

    ui->startStopButton->setText("Start");
}

void MainWindow::log(QString text)
{
    // Remove extra newlines for headers to be printed neatly
    if (text.endsWith("\n")) text.chop(1);
    if (text.endsWith('\r')) text.chop(1);
    ui->transferLog->appendPlainText(text);
}


