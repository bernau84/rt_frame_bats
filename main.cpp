#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QAudioInput>
#include <QAudioDeviceInfo>

//#include "analysis\rt_cpb_qt.h"
#include "analysis\rt_filter_qt.h"
#include "analysis\rt_w_pwr_qt.h"
#include "inputs\rt_snd_in_qt.h"
#include "inputs\rt_wav_in_qt.h"
#include "outputs\rt_snd_out_qt.h"

#include "inputs\wav_read_file.h"
#include "outputs\wav_write_file.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QList<QAudioDeviceInfo> infos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i=0; i<infos.count(); i++)
        qDebug() << infos[i].deviceName();

    //rt_snd_out_fp sndout(QAudioDeviceInfo::defaultOutputDevice());
    rt_wav_in_fp wavin;

    //sndout.start();
    wavin.start();


    //rt_snd_in_fp *aidev = new rt_snd_in_fp(); //berem prvni dobrou
    //rt_cpb_fp *aibat = new rt_cpb_fp(); //navazame na vzorkovac
    //rt_filter_fp *aifilt = new rt_filter_fp(); //navazame na vzorkovac
    //rt_w_pwr_fp *ai_w_pwr = new rt_w_pwr_fp(); //navazame na vzorkovac
    //rt_snd_out_fp *aodev = new rt_snd_out_fp(); //audio output

    //aibat->connection(aidev);
    //aifilt->connection(aidev);
    //ai_w_pwr->connection(aidev);
    //aodev->connection(aidev);

    /*
    double half_band_coe[] = {
         #include "analysis\filter_half_b_cheby_fir_coe2.h";
    };

    unsigned half_band_coe_N = sizeof(half_band_coe) / sizeof(double);
    T half_band_coe_T[half_band_coe_N];
    for(int i=0; i<half_band_coe_N; i++)  half_band_coe_T[i] = T(half_band_coe[i]);
    */

    /*
    t_waw_file_reader wiex("c:\\mares\\Audio\\10multi_harm_sin_08.wav", true);
    t_waw_file_reader wiex("c:\\Users\\bernau84\\Documents\\sandbox\\simulace\\chirp_20_8000_fs16kHz.wav", true);
    t_waw_file_reader::t_wav_header inf; wiex.info(inf);

    t_waw_file_writer woex("c:\\mares\\Audio\\10multi_harm_sin_08_copy.wav", 8000, 1, 8);

    for(int i=0; i<2000; i++){

        double tmp[300];
        wiex.read(tmp, 300);
        woex.write(tmp, 300);
    }
    */

    //aibat->start();
    //aifilt->start();
    //ai_w_pwr->start();
    //aodev->start();
    //aidev->start(); //cele to odstartujem

    return a.exec();
}
