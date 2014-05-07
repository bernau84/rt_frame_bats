#include "rt_sources.h"
#include "rt_basictypes.h"
#include <QtDebug>

t_rt_sources::t_rt_sources(QObject *parent):
    t_rt_base(parent)
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_snd_card::t_rt_snd_card(const QAudioDeviceInfo &in, QObject *parent):
    t_rt_sources(parent), input_dev(in)
{
    t_setup_entry fr(in.supportedFrequencies().toJson().toArray(), "Hz");  //recent list
    int fr_a = set.ask("Rates")->get().toInt();  //actual frequency
    set.insert("Rates", fr);  //update list
    fr.set(fr_a); //select original frequnecy of default if doesnt exist

    //zvolime defaultni vzorkovacku
    sta.fs_out = sta.fs_in = set.ask("Rates")->get(); //vyctem

    input_io = 0;
    input_audio = 0;

    change();
}


void t_rt_snd_card::start(){

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
                if((input_io = input_audio->start()));  //zapiname pull mode
                    connect(input_io, SIGNAL(readyRead()), SLOT(process()));
                break;
            case QAudio::IdleState:
                break;  //nic - cekame na data
        }
}

void t_rt_snd_card::pause(){

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

    double scale = 1.0 / (1 << (format.sampleSize()-1));  //mame to v signed
    for(int i=0; i<readable_l; i++){  //konverze do double a zapis

        wrks.f[wrks.i] = nn_tot / *sta.fs_in; //vkladame cas kazdeho vzorku
        wrks.A[wrks.i] = scale*local_samples[i];
        wrks.i += 1;
        nn_tot += 1;  //celkovy pocet zpracovanych vzorku

        if(wrks.i == wrks.A.count()) {

            t_slcircbuf::write(wrks); //zapisem novy jeden radek
            t_slcircbuf::readShift(1); //a novy pracovni si hned vyctem
            t_slcircbuf::get(&wrks, 1);
            wrks.i = 0; //jdem od zacatku
            wrks.t = nn_tot / *sta.fs_in; //predpoklad konstantnich t inkrementu; cas 1. ho vzorku
        }
    }

    t_slcircbuf::set(&wrks, 1);   //zapisem zpet i kdyz neni cely
    emit on_update(); //dame vedet dal
}

void t_rt_snd_card::change_audio_state(QAudio::State act){

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

    sta.fs_out = format.frequency();
    sta.fs_in = &sta.fs_out;   //dany predcozi prvek def. vzorkovacku neni

    int N = set["Multibuffer"].get().toInt();
    int M = set["Refresh"].get().toInt() / 1000.0 * sta.fs_out;

    t_slcircbuf::resize(M); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    t_rt_slice dfs;
    dfs.A = QVector<double>(N, 0);
    dfs.f = QVector<double>(N, 0);
    dfs.i = 0;
    dfs.t = 0.0; //vse na 0

    t_slcircbuf::init(dfs); //nastavime vse na stejno
    t_slcircbuf::clear(); //vynulujem ridici promenne - zacnem jako po startu na inx 0

    input_audio->setNotifyInterval(set.refreshRt[set.refreshI].v);  //navic mame to od byteready
    connect(input_audio, SIGNAL(notify()), SLOT(process()));
    connect(input_audio, SIGNAL(stateChanged(QAudio::State)), SLOT(change_audio_state(QAudio::State)));   //s chybou prechazi AudioInput do stavi Stopped

    emit on_change();
}
