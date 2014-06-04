#include "rt_sources.h"
#include "rt_basictypes.h"
#include <QtDebug>

t_rt_sources::t_rt_sources(QObject *parent, const QDir &resource):
    t_rt_base(parent, resource)
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_snd_card::t_rt_snd_card(const QAudioDeviceInfo &in, QObject *parent):
    t_rt_sources(parent, QDir(":/config/js_config_sndsource.txt")), input_dev(in)
{
    QJsonArray jfrl;  //conversion
    QList<int> qfrl = in.supportedSampleRates();
    foreach(int f, qfrl) jfrl.append(f);
    t_setup_entry fr(jfrl, "Hz");  //recent list
    sta.fs_out = set["Rates"].get().toDouble();  //actual frequency

    set.replace("Rates", fr);  //update list
    sta.fs_out = set["Rates"].set(sta.fs_out).toDouble(); //select original frequnecy of default if doesnt exist

    *sta.fs_in = sta.fs_out;  //on source is io fr equal

    input_io = 0;
    input_audio = 0;

    change();
}


void t_rt_snd_card::start(){

    t_rt_base::start();

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

void t_rt_snd_card::pause(){

    t_rt_base::pause();

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

void t_rt_snd_card::process(){

    if(!input_io || !input_audio)
        return;

    qint64 avaiable_l = input_audio->bytesReady();//input_io->bytesAvailable();
    short local_samples[avaiable_l];  //vycteni dostupneho
    qint64 readable_l = input_io->read((char *)local_samples, avaiable_l);
    if(!readable_l) return;

    t_rt_slice wrks; //pracovni radek v multibufferu
    t_slcircbuf::get(&wrks, 1);  //vyctem

    int M = set["Refresh"].get().toDouble() / 1000.0 * sta.fs_out;

    double scale = 1.0 / (1 << (format.sampleSize()-1));  //mame to v signed
    for(int i=0; i<readable_l; i++){  //konverze do double a zapis

        wrks.v << t_rt_slice::t_rt_tf(*sta.fs_in/2, scale*local_samples[i]); //vkladame akt. frekvenci a amplitudu
            //akt. frekvence okamzite amp;litudy odpovida nyquistovce
        if(wrks.v.size() == M) {

            t_slcircbuf::write(wrks); //zapisem novy jeden radek
            t_slcircbuf::read(&wrks); //a novy pracovni si hned vyctem
            wrks = t_rt_slice(sta.nn_run / *sta.fs_in, M); //predpoklad konstantnich t inkrementu; cas 1. ho vzorku
        }

        sta.nn_tot += 1;  //celkovy pocet zpracovanych vzorku
        sta.nn_run += 1;
    }

    t_slcircbuf::set(&wrks, 1);   //zapisem zpet i kdyz neni cely
    emit on_update(); //dame vedet dal
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
    sta.fs_out = set["Rates"].get().toDouble();  //actual frequency
    int N = set["Multibuffer"].get().toDouble();
    int M = set["Refresh"].get().toDouble() / 1000.0 * sta.fs_out;

    t_slcircbuf::resize(N, true); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    t_rt_slice dfs(0, M);
    t_slcircbuf::set(&dfs); //pripravime novy rez

    input_audio->setNotifyInterval(set["Refresh"].get().toDouble());  //navic mame to od byteready
    connect(input_audio, SIGNAL(notify()), SLOT(process()));
    connect(input_audio, SIGNAL(stateChanged(QAudio::State)), SLOT(change_audio_state(QAudio::State)));   //s chybou prechazi AudioInput do stavi Stopped

    emit on_change();
}
