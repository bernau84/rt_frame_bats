#include "rt_sources.h"
#include "../base/rt_dataflow.h"

#include <QtDebug>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_snd_card::t_rt_snd_card(const QAudioDeviceInfo &in, QObject *parent):
    rt_node(parent),
    t_rt_source<double>(parent, QDir(":/config/js_config_sndsource.txt")),
    input_dev(in)
{
    QJsonArray jfrl;  //conversion
    QList<int> qfrl = in.supportedSampleRates();
    foreach(int f, qfrl) jfrl.append(f);
    t_setup_entry fr(jfrl, "Hz");  //recent list
    sta.fs_out = par["Rates"].get().toDouble();  //actual frequency

    par.replace("Rates", fr);  //update list
    sta.fs_out = par["Rates"].set(sta.fs_out).toDouble(); //select original frequnecy of default if doesnt exist

    *sta.fs_in = sta.fs_out;  //on source is io fr equal

    input_io = 0;
    input_audio = 0;
}


void t_rt_snd_card::start(){

    rt_node::start();

    if(input_audio)
        switch(input_audio->state()){

            case QAudio::ActiveState:
                input_audio->reset();  //drop unread data
                break;
            case QAudio::SuspendedState:
                input_audio->resume();  //resume
                break;
            case QAudio::StoppedState:
                if(input_io) disconnect(input_io, 0, this, 0);
                if((input_io = input_audio->start()))  //zapiname pull mode
                    connect(input_io, SIGNAL(readyRead()), SLOT(process()));
                break;
            case QAudio::IdleState:
                break;  //nic - cekame na data
        }
}

void t_rt_snd_card::stop(){

    rt_node::stop();

    if(input_audio)
        switch(input_audio->state()){

            case QAudio::ActiveState:
                input_audio->suspend();  //pause
                break;
            case QAudio::SuspendedState:
            case QAudio::StoppedState:
                break;      //nothing
            case QAudio::IdleState:
                input_audio->suspend();  //pause
                break;
        }
}

void t_rt_snd_card::process(const void *dt, int size){

    if(!input_io || !input_audio)
        return;

    qint64 avaiable_l = input_audio->bytesReady();//input_io->bytesAvailable();
    short local_samples[avaiable_l];  //vycteni dostupneho
    qint64 readable_l = input_io->read((char *)local_samples, avaiable_l);
    if(!readable_l) return;

    t_rt_slice<double> w; //pracovni radek v multibufferu
    get(w);  //vyctem

    double frres = 2 / *sta.fs_in; //fr rozliseni == nyquistovu frekvenci
    double scale = 1.0 / (1 << (format.sampleSize()-1));  //mame to v signed

    for(int i=0; i<readable_l; i++){  //konverze do double a zapis

        if(w.isempty())  //predpoklad konstantnich t inkrementu; vkladame cas 1. ho vzorku
            w = t_rt_slice<double>(sta.nn_run / *sta.fs_in, M);

        if(0 == w.append(t_rt_slice::t_rt_ai(frres, scale*local_samples[i])))  //slice is full
            write(w); //zapisem novy jeden radek

        sta.nn_tot += 1;  //celkovy pocet zpracovanych vzorku od zmeny
        sta.nn_run += 1;   //total - jen inkrementujem, kvuli statistice
    }

    set(w);   //zapisem zpet i kdyz neni cely
}

void t_rt_snd_card::change_audio_state(QAudio::State act){

    Q_UNUSED(act);
//    if(input_audio) //bylo uz zinicializovano?
//        if(input_audio->state() == QAudio::StoppedState)
//            error_audio_state(); //diagnostika chyb
}

void t_rt_snd_card::error_audio_state(void){

    if(input_audio) //bylo uz zinicializovano?
        switch(input_audio->error()){

            case QAudio::NoError:       //No errors have occurred
                break;
            case QAudio::OpenError:	//An error opening the audio device
            case QAudio::IOError:	//An error occurred during read/write of audio device
            case QAudio::UnderrunError:	//Audio data is not being fed to the audio device at a fast enough rate
            case QAudio::FatalError:	//A non-recoverable error has occurred, the audio device is not usable at this time.

                qDebug() << this->objectName() << ", error!";
                return;
        }
}

void t_rt_snd_card::change(){

//    format.setByteOrder(QAudioFormat::LittleEndian); //pevne
//    format.setChannelCount(1); //pevne
//    format.setCodec("audio/pcm"); //pevne
//    format.setSampleSize(16); //na tvrdo
//    format.setSampleRate(set.f.freqList[set.f.freqIndex].v);
//    format.setSampleType(QAudioFormat::SignedInt);

//    if (!info.isFormatSupported(format)) {
//        qWarning()<<"default format not supported try to use nearest";
//        format = info.nearestFormat(format);
//    }

    if(input_audio) delete input_audio;
    if(0 == (input_audio = new QAudioInput(input_dev, format, this))){

        qDebug() << this->objectName() << ", error!";
        return; //tak to neklaplo - takovy format nemame
    }

    sta.nn_run = 0;
    sta.fs_out = par["Rates"].get().toDouble();  //actual frequency
    N = par["Multibuffer"].get().toDouble();
    M = par["Refresh"].get().toDouble();

    resize(N); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    t_rt_slice<double> dfs(0);
    set(dfs); //vymazem akt. slice aby se mohl s novymi daty inicializovat

    input_audio->setNotifyInterval(M);  //navic mame to od byteready
    connect(input_audio, SIGNAL(notify()), SLOT(process()));
    connect(input_audio, SIGNAL(stateChanged(QAudio::State)), SLOT(change_audio_state(QAudio::State)));   //s chybou prechazi AudioInput do stavi Stopped

    M *= (sta.fs_out  / 1000.0); //conversion to sample number
}
