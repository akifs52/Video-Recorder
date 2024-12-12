#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVariant>
#include <QComboBox>
#include <QAudioDevice>
#include <QWindow>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , audioformat(new QAudioFormat)
    , audioInput(nullptr)
    , timer(new QTimer(this))
    , ffmpegProcess(new QProcess(this)) // FFmpeg işlemi için QProcess
{
    ui->setupUi(this);

    // QAudioDevice tipi için QVariant kullanıma hazır hale getiriliyor
    audioformat->setSampleRate(44100);
    audioformat->setChannelCount(2);
    audioformat->setSampleFormat(QAudioFormat::Int16);

    populateAudioInputDevices(); // Mevcut ses giriş aygıtlarını listele

    ui->durationLabel->setText("No Recording");

    ui->frame_5->hide();

}

MainWindow::~MainWindow()
{
    delete ui;
    if (ffmpegProcess->state() == QProcess::Running) {
        qDebug() << "Kayıt durduruluyor...";

        // ffmpeg'e özel bir kapatma sinyali gönder
        ffmpegProcess->write("q");  // FFmpeg'e "q" göndererek kaydı bitirmesini istiyoruz
        ffmpegProcess->closeWriteChannel();  // Yazma kanalını kapat, FFmpeg sonlandırmayı işlesin

        if (!ffmpegProcess->waitForFinished(5000)) {  // 5 saniye bekle, hala durmadıysa...
            qDebug() << "FFmpeg işlemi düzgün şekilde sonlandırılamadı, zorla kapatılıyor...";
            ffmpegProcess->kill();  // Zorla sonlandırmak en son çare
            ffmpegProcess->waitForFinished();
        }

        timer->stop();  // Ekran görüntüsü güncellemeyi durdur
        delete elapsedTimer; //kayıt durdurulunca sil
        ui->durationLabel->setText("Recording stop");
        qDebug() << "Kayıt durduruldu!";
        disconnect(timer, &QTimer::timeout, this, &MainWindow::updateFrame);
        disconnect(durationTimer,&QTimer::timeout,this,&MainWindow::updateRecordingDuration);
        QMessageBox::warning(this,"Recording Stoped","The recording stoped");
        totalMiliseconds = 0;
    }
}

// Ses ve Ekran kaydını başlatan düğme
void MainWindow::on_start_clicked()
{
    int index = ui->audioInputSelect->currentIndex();

    if (index < 0) {
        return; // Ses girişi seçilmediyse kayda başlamaz
    }

    QAudioDevice selectedDevice = ui->audioInputSelect->currentData().value<QAudioDevice>();
    QString selectedInputDevice = selectedDevice.description(); // Veya selectedDevice.id();

    qDebug() << "Selected Audio Device:" << selectedInputDevice;

    if(timerbool)
    {
        on_radioButton_clicked(Qt::Checked);
    }

    // Kayıt edilecek dosya için bir yol seçin
    QString outputFile = QFileDialog::getSaveFileName(this, "Kayıt Dosyasını Seç", "", "Video Files (*.mp4)");

    if (outputFile.isEmpty()) {
        return; // Dosya seçilmediyse başlatma
    }

    // FFmpeg komutunu çalıştırma
    QString program = "ffmpeg";

    QStringList arguments;

    arguments << "-f" << "gdigrab"               // Windows masaüstü yakalama
              << "-i" << "desktop"               // Masaüstü giriş kaynağı
              << "-f" << "dshow"                 // DirectShow ses girişi
              << "-i" << "audio=" + selectedInputDevice  // Mikrofon kaynağı
              << "-f" << "dshow"                 // DirectShow ses girişi (Stereo Mix)
              << "-i" << "audio=VoiceMeeter Output (VB-Audio VoiceMeeter VAIO)"  // Sistem sesleri
              << "-filter_complex" << "[1:a][2:a]amerge=inputs=2[aout]" // Mikrofon ve sistem seslerini birleştirme
              << "-map" << "0:v"                 // Video kaynağını kullanma
              << "-map" << "[aout]"              // Birleştirilmiş ses kaynağını kullanma
              << "-vf" << "scale=1920:1080"      // Çözünürlüğü 1920x1080'e ayarlama
              << "-r" << "30"                    // 30 FPS kare hızını ayarlama
              << "-c:v" << "h264_nvenc"          // NVIDIA GPU hızlandırmalı video kodlama
              << "-preset" << "slow"             // Kodlama hızı (kalite odaklı, 'slow')
              << "-cq:v" << "20"                 // Video kalitesini ayarlamak için Constant Quality
              << "-c:a" << "aac"                 // Ses için AAC kodlama
              << outputFile;                     // Çıkış dosyası
    ffmpegProcess->start(program, arguments);

    if (!ffmpegProcess->waitForStarted()) {
        qDebug() << "FFmpeg başlatılamadı!";
        return;
    }

    qDebug() << "Ekran ve ses kaydı başladı!";



    connect(timer,&QTimer::timeout,this,&MainWindow::updateFrame);

    timer->start(30);


    elapsedTimer = new QElapsedTimer();

    elapsedTimer->start();

    durationTimer = new QTimer();

    connect(durationTimer,&QTimer::timeout,this,&MainWindow::updateRecordingDuration);

    durationTimer->start(1000);

    // Süre dolduğunda kaydı otomatik olarak durduracak zamanlayıcıyı ayarlıyoruz
    if(timerbool)
    {
         QTimer::singleShot(totalMiliseconds, this, &MainWindow::on_stop_clicked);
    }



}

// Kaydı durdurma düğmesi
void MainWindow::on_stop_clicked()
{
    if (ffmpegProcess->state() == QProcess::Running) {
        qDebug() << "Kayıt durduruluyor...";

        // ffmpeg'e özel bir kapatma sinyali gönder
        ffmpegProcess->write("q");  // FFmpeg'e "q" göndererek kaydı bitirmesini istiyoruz
        ffmpegProcess->closeWriteChannel();  // Yazma kanalını kapat, FFmpeg sonlandırmayı işlesin

        if (!ffmpegProcess->waitForFinished(5000)) {  // 5 saniye bekle, hala durmadıysa...
            qDebug() << "FFmpeg işlemi düzgün şekilde sonlandırılamadı, zorla kapatılıyor...";
            ffmpegProcess->kill();  // Zorla sonlandırmak en son çare
            ffmpegProcess->waitForFinished();
        }

        timer->stop();  // Ekran görüntüsü güncellemeyi durdur
        delete elapsedTimer; //kayıt durdurulunca sil
        ui->durationLabel->setText("Recording stop");
        qDebug() << "Kayıt durduruldu!";
        disconnect(timer, &QTimer::timeout, this, &MainWindow::updateFrame);
        disconnect(durationTimer,&QTimer::timeout,this,&MainWindow::updateRecordingDuration);
        QMessageBox::warning(this,"Recording Stoped","The recording stoped");
        totalMiliseconds = 0;
    }


}

// Ses cihazlarını listeleyen fonksiyon
void MainWindow::populateAudioInputDevices()
{
    ui->audioInputSelect->clear();

    const auto devices = QMediaDevices::audioInputs();
    for (const QAudioDevice &device : devices)
    {
        ui->audioInputSelect->addItem(device.description(), QVariant::fromValue(device));

    }
}

void MainWindow::updateFrame()
{
    QScreen *screen = QGuiApplication::primaryScreen(); //ana ekranı seçiyoruz

    if(const QWindow *window = windowHandle())
    {
        screen = window->screen();
    }
    if(!screen)
    {
        return;
    }

    QPixmap pixmap = screen->grabWindow(0); //tüm ekranı yakalar
    ui->label->setPixmap(pixmap.scaled(ui->label->size(),Qt::KeepAspectRatio));


}

void MainWindow::updateRecordingDuration()
{
    qint64 elapsed = elapsedTimer->elapsed();

    int seconds = (elapsed/1000) % 60; //milisaniyeyi saniyeye çevirdik ve 60 la modunu aldık tam sayı olsun diye
    int minutes = (elapsed/60000) % 60;
    int hours = (elapsed/3600000);

    QString timeText = QString("%1:%2:%3")
                       .arg(hours,2,10,QChar('0'))
                       .arg(minutes,2,10,QChar('0'))
                       .arg(seconds,2,10,QChar('0'));

    ui->durationLabel->setText("Record Duration: "+timeText);

}

void MainWindow::on_radioButton_clicked(bool checked)
{
    if(checked)
    {
        //alınan video uzunluğu
        int hours = ui->hourBox->value();
        int minutes = ui->minutesBox->value();
        int seconds = ui->secondBox->value();

        //milisaniyeye çeviriyoruz
        totalMiliseconds = ((hours*60*60)+(minutes*60)+seconds)*1000;
        timerbool = true;

    }
    else
    {
        timerbool = false;
        return;

    }
}

