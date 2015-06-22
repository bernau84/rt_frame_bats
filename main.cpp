#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QAudioInput>
#include <QAudioDeviceInfo>

#include "analysis\rt_cpb_qt.h"
#include "inputs\rt_snd_in_qt.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QList<QAudioDeviceInfo> infos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i=0; i<infos.count(); i++)
        qDebug() << infos[i].deviceName();

    if(infos.count()){

        rt_snd_in_fp *aidev = new rt_snd_in_fp(); //berem prvni dobrou
        rt_cpb_fp *aibat = new rt_cpb_fp(); //navazame na vzorkovac
        aibat->connection(aidev);

        aibat->start();
        aidev->start(); //cele to odstartujem
    }

    return a.exec();
}
