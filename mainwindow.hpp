#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QFutureWatcher>
#include <QList>
#include <QMainWindow>

class QDragEnterEvent;
class QDropEvent;
class QFileInfo;
class QImage;

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
    void on_buttonBrowseInDir_clicked();

    void on_buttonResize_clicked();

    void on_buttonClear_clicked();

    void resizeFinished();

    void on_comboBox_activated(const QString &arg1);

private:
    Ui::MainWindow *ui;

    QList<QPair<QFileInfo, QImage> > images;

    QFutureWatcher<void> futureWatcher;

    bool enqueue(const QFileInfo& fileInfo);

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

#endif // MAINWINDOW_HPP
