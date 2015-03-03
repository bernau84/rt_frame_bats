#include <QApplication>
#include "mainwindow.h"
#include <QObject>
#include <QWidget>
#include <QDebug>
#include <QAudioInput>
#include <QAudioDeviceInfo>
#include <QPushButton>
#include "inputs\rt_sources.h"
#include "analysis\rt_analysis.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QList<QAudioDeviceInfo> infos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i=0; i<infos.count(); i++)
        qDebug() << infos[i].deviceName();

    if(infos.count()){

        t_rt_snd_card *aidev = new t_rt_snd_card(infos[0/*8*/]); //berem prvni dobrou
        t_rt_shift *aibat = new t_rt_shift(); //navazame na vzorkovac
        aibat->attach(aidev);

        aibat->start();
        aidev->start(); //cele to odstartujem
    }

    w.show();
    return a.exec();
}
