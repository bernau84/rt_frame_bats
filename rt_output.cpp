#include "rt_output.h"

t_rt_output::t_rt_output(QObject *parent, const QDir &resource) :
    t_rt_base(parent, resource)
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_player::t_rt_player(const QAudioDeviceInfo &out, QObject *parent):
    t_rt_output(parent, QDir(":/config/js_config_sndsink.txt")), output_dev(out)
{
    QJsonArray jfrl;  //conversion
    QList<int> qfrl = out.supportedSampleRates();
    foreach(int f, qfrl) jfrl.append(f);
    t_setup_entry fr(jfrl, "Hz");  //recent list
    sta.fs_out = set["Rates"].get().toDouble();  //actual frequency

    output_io = 0;
    output_audio = 0;

    change();
}


void t_rt_player::start(){

    if(output_audio)
        switch(output_audio->state()){

            case QAudio::ActiveState:
                output_audio->reset();  //drop unread data
                break;
            case QAudio::SuspendedState:
                output_audio->resume();  //resume
                break;
            case QAudio::StoppedState:
                if(output_io) disconnect(output_io, 0, this, 0);
                if((output_io = output_audio->start()));  //zapiname pull mode
                    connect(output_io, SIGNAL(readyRead()), SLOT(process()));
                break;
            case QAudio::IdleState:
                break;  //nic - cekame na data
        }
}

void t_rt_player::pause(){

    if(output_audio)
        switch(output_audio->state()){

            case QAudio::ActiveState:
                output_audio->suspend();  //pause
                break;
            case QAudio::SuspendedState:
            case QAudio::StoppedState:
                break;      //nothing
            case QAudio::IdleState:
                output_audio->suspend();  //pause
                break;
        }
}

/*! \brief - konvertuje vzorky do formatu prehravace, sype to do nej a ty ktere stiha
 * prehrat presypava do multibufferu */
void t_rt_player::process(){

    if(!output_io || !output_audio)
        return;

    int M = set["Time"].get().toDouble() / 1000.0 * sta.fs_out;

    t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
    if(!pre) return; //navazujem na zdroj dat?

    t_rt_slice wrks; //pracovni radek v multibufferu
    t_slcircbuf::get(&wrks, 1);  //vyctem

    int scale = 1 << (format.sampleSize()-1);  //mame to v signed proto -1

    t_rt_slice t_amp;  //radek casovych dat
    while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //vyctem radek

        if(sta.state == t_rt_status::ActiveState){

            qint64 avaiable_l = output_audio->bytesFree();
            if(avaiable_l < t_amp.avail){ ; }  /*! \todo - !!nestihame prehravat --> zahazujem vzorky */
            else if(avaiable_l > t_amp.avail) avaiable_l = t_amp.avail;  //stihame; staci nam jen prostor na jeden radek

            short local_samples[avaiable_l];  //vycteni dostupneho
            for(int i=0; i < avaiable_l; i++){  //konverze do shortu a zapis

                local_samples[i] = t_amp.v[i].A * scale;
                wrks.v << t_rt_slice::t_rt_tf(sta.fs_out/2, local_samples[i]); //vkladame cas kazdeho vzorku
                if(wrks.v.size() == M){

                    t_slcircbuf::write(wrks); //zapisem novy jeden radek
                    t_slcircbuf::read(&wrks, 1);
                    wrks = t_rt_slice(sta.nn_tot / *sta.fs_in); //predpoklad konstantnich t inkrementu; cas 1. ho vzorku
                }

                sta.nn_tot += 1;
                sta.nn_run += 1;
            }

            qint64 written_l = output_io->write((char *)local_samples, avaiable_l);
            (void)written_l;
         }
    }

    t_slcircbuf::set(&wrks, 1);   //zapisem zpet i kdyz neni cely
    emit on_update(); //dame vedet dal
}

void t_rt_player::change_audio_state(QAudio::State act){

//    if(input_audio) //bylo uz zinicializovano?
//        if(input_audio->state() == QAudio::StoppedState)
//            error_audio_state(); //diagnostika chyb
}

void t_rt_player::error_audio_state(void){

    if(output_audio) //bylo uz zinicializovano?
        switch(output_audio->error()){

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

void t_rt_player::change(){

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

    if(output_audio) delete output_audio;
    if(0 == (output_audio = new QAudioOutput(output_dev, format, this))){

        qDebug() << this->objectName() << ", error!";
        return; //tak to neklaplo - takovy format nemame
    }

    sta.fs_out = set["Rates"].get().toDouble();  //actual frequency
    int N = set["Multibuffer"].get().toDouble();
    int M = set["Time"].get().toDouble() / 1000.0 * sta.fs_out;

    t_slcircbuf::resize(M, true); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    t_rt_slice dfs(0);
    t_slcircbuf::set(&dfs, 1);

    int mmrt = set["__refresh_rate"].get().toDouble() * sta.fs_out * 1000;
    output_audio->setNotifyInterval(mmrt); //v ms
    connect(output_audio, SIGNAL(notify()), SLOT(process()));
    connect(output_audio, SIGNAL(stateChanged(QAudio::State)), SLOT(change_audio_state(QAudio::State)));   //s chybou prechazi AudioInput do stavi Stopped

    emit on_change();
}


