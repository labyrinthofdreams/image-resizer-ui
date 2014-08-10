#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QMimeData>
#include <QPair>
#include <QString>
#include <QtConcurrent>
#include <QUrl>
#include "mainwindow.hpp"
#include "ui_mainwindow.h"

static QMap<QString, int> resolutionWidth {{"DVD (NTSC)", 700}, {"DVD (PAL)", 720},
                                            {"Widescreen DVD", 720}, {"HD 720p", 1280},
                                            {"HD 1080p", 1920}, {"HD 2k Flat", 1998},
                                            {"HD 2k", 2048}, {"UHD 4k", 4096}, {"UHD 8k", 7680}};

static QMap<QString, int> resolutionHeight {{"DVD (NTSC)", 480}, {"DVD (PAL)", 576},
                                            {"Widescreen DVD", 480}, {"HD 720p", 720},
                                            {"HD 1080p", 1080}, {"HD 2k Flat", 1080},
                                            {"HD 2k", 1080}, {"UHD 4k", 2160}, {"UHD 8k", 4320}};

class ScaleToWidth {
    int _width;

public:
    ScaleToWidth(int width) : _width(width) {}
    void operator()(QPair<QFileInfo, QImage>& image) {
        image.second = image.second.scaledToWidth(_width, Qt::SmoothTransformation);
    }
};

class ScaleToHeight {
    int _height;

public:
    ScaleToHeight(int height) : _height(height) {}
    void operator()(QPair<QFileInfo, QImage>& image) {
        image.second = image.second.scaledToHeight(_height, Qt::SmoothTransformation);
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    images()
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_buttonBrowseInDir_clicked()
{
    const QString inputDirectory =
            QFileDialog::getExistingDirectory(this, tr("Select input directory"));
    if(inputDirectory.isEmpty()) {
        return;
    }

    QDir inDir(inputDirectory);
    const QFileInfoList fileList = inDir.entryInfoList(QDir::Files | QDir::Readable |
                                                       QDir::NoDotAndDotDot);
    int saved = 0;
    for(const auto& file : fileList) {
        if(enqueue(file)) {
            ++saved;
        }
    }

    ui->plainTextEditLog->appendPlainText(tr("Added %1 images from %2").arg(saved).arg(inDir.absolutePath()));
    ui->statusBar->showMessage(tr("Images to process: %1").arg(images.size()));
}

bool MainWindow::enqueue(const QFileInfo& fileInfo)
{
    QImage image;
    if(image.load(fileInfo.absoluteFilePath())) {
        images.append(qMakePair(fileInfo, image));
        return true;
    }

    return false;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    int saved = 0;
    const QList<QUrl> urls = event->mimeData()->urls();
    for(const auto& url : urls) {
        const QFileInfo local(url.toLocalFile());
        if(enqueue(local)) {
            ++saved;
        }
    }

    ui->plainTextEditLog->appendPlainText(tr("Added %1 images").arg(saved));
    ui->statusBar->showMessage(tr("Images to process: %1").arg(images.size()));

    event->acceptProposedAction();
}

void MainWindow::on_buttonResize_clicked()
{
    const auto outputPath =
            QFileDialog::getExistingDirectory(this, tr("Select output directory"));
    if(outputPath.isEmpty()) {
        return;
    }

    if(images.empty()) {
        QMessageBox::warning(this, tr("Nothing to process"),
                             tr("Add images and try again"));
        return;
    }

    // Resize images
    ui->plainTextEditLog->appendPlainText(tr("Resizing images..."));
    if(ui->radioResizeWidth->isChecked()) {
        QtConcurrent::blockingMap(images.begin(), images.end(),
                                  ScaleToWidth(ui->spinBoxResizeTo->value()));

    }
    else if(ui->radioResizeHeight->isChecked()) {
        QtConcurrent::blockingMap(images.begin(), images.end(),
                                  ScaleToHeight(ui->spinBoxResizeTo->value()));
    }

    ui->plainTextEditLog->appendPlainText(tr("Saving images..."));

    // Save to output directory
    const auto formatIdx = ui->comboBoxFormat->currentIndex();
    const QString format = ui->comboBoxFormat->currentText();
    const auto quality = ui->spinBoxQuality->value();
    int saved = 0;
    const QDir outDir(outputPath);
    for(const auto& image : images) {
        const QFileInfo& fileInfo = image.first;
        const auto fileNameNoSuffix = fileInfo.completeBaseName();
        QString suffix = fileInfo.suffix();
        if(format == "JPEG") {
            suffix = "jpg";
        }
        else if(format == "PNG") {
            suffix = "png";
        }

        const QString fileName = QString("%1.%2").arg(fileNameNoSuffix).arg(suffix);
        const QString out = outDir.absoluteFilePath(fileName);
        qDebug() << out << format.toLatin1().constData();
        if(ui->checkBoxOverwrite->isChecked() || !QFile::exists(out)) {
            if(formatIdx == 0) {
                // Auto-detect format
                image.second.save(out, 0, quality);
            }
            else {
                image.second.save(out, format.toLatin1().constData(), quality);
            }
            ++saved;
        }
    }

    ui->plainTextEditLog->appendPlainText(tr("Processed %1 images (skipped: %2)")
                                          .arg(saved).arg(images.size() - saved));
    ui->statusBar->showMessage(tr("Processed %1 of %2 images")
                               .arg(saved).arg(images.size()));
}

void MainWindow::on_buttonClear_clicked()
{
    images.clear();

    ui->plainTextEditLog->clear();
    ui->statusBar->showMessage(tr("Images to process: 0"));
}

void MainWindow::on_comboBox_activated(const QString &arg1)
{
    if(ui->radioResizeWidth->isChecked()) {
        ui->spinBoxResizeTo->setValue(resolutionWidth.value(arg1));
    }
    else if(ui->radioResizeHeight->isChecked()) {
        ui->spinBoxResizeTo->setValue(resolutionHeight.value(arg1));
    }
}
