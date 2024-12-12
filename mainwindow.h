#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaDevices>
#include <QAudioInput>
#include <QAudioFormat>
#include <QFile>
#include <QTimer>
#include <QProcess>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_start_clicked();

    void on_stop_clicked();

    void populateAudioInputDevices();

    void updateFrame();

    void updateRecordingDuration();
    void on_radioButton_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    QAudioFormat *audioformat;
    QAudioInput *audioInput;

    QTimer *timer;

    QElapsedTimer *elapsedTimer;
    QTimer *durationTimer;

    QProcess *ffmpegProcess;

    bool timerbool = false;
    int totalMiliseconds;//video süresi için
};
#endif // MAINWINDOW_H
