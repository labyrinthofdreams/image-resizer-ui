#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QPair>
#include <QString>
#include <QtConcurrent>
#include <QUrl>
#include "mainwindow.hpp"
#include "ui_mainwindow.h"

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
    if(images.empty()) {
        QMessageBox::warning(this, tr("Nothing to process"),
                             tr("Add images and try again"));
        return;
    }

    const auto outputPath =
            QFileDialog::getExistingDirectory(this, tr("Select output directory"));
    if(outputPath.isEmpty()) {
        return;
    }

    // Resize images
    if(ui->checkBoxResizeTo->isChecked()) {
        if(ui->radioResizeWidth->isChecked()) {
            QtConcurrent::blockingMap(images.begin(), images.end(), ScaleToWidth(ui->lineEditResize->text().toInt()));
        }
        else if(ui->radioResizeHeight->isChecked()) {
            QtConcurrent::blockingMap(images.begin(), images.end(), ScaleToHeight(ui->lineEditResize->text().toInt()));
        }
    }

    // Save to output directory
    const auto quality = ui->spinBoxQuality->value();
    int saved = 0;
    const QDir outDir(outputPath);
    for(const auto& image : images) {
        const QFileInfo& fileInfo = image.first;
        const QString out = outDir.absoluteFilePath(fileInfo.fileName());
        if(ui->checkBoxOverwrite->isChecked() || !QFile::exists(out)) {
            image.second.save(out, 0, quality);
            ++saved;
        }
    }

    ui->plainTextEditLog->appendPlainText(tr("Processed %1 images (Skipped %2)")
                                          .arg(saved).arg(images.size() - saved));
    ui->statusBar->showMessage(tr("Processed %1 of %2 images")
                               .arg(saved).arg(images.size()));

    if(images.size() == saved) {
        images.clear();
    }
}

void MainWindow::on_buttonClear_clicked()
{
    images.clear();

    ui->plainTextEditLog->clear();
    ui->statusBar->showMessage(tr("Images to process: 0"));
}
