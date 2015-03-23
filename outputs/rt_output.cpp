#include "rt_output.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_player::t_rt_player(const QAudioDeviceInfo &out, QObject *parent):
    rt_node(parent),
    t_rt_output(parent, QDir(":/config/js_config_sndsink.txt")),
    output_dev(out)
{
    QJsonArray jfrl;  //conversion
    QList<int> qfrl = out.supportedSampleRates();
    foreach(int f, qfrl) jfrl.append(f);
    t_setup_entry fr(jfrl, "Hz");  //recent list
    sta.fs_out = par["Rates"].get().toDouble();  //actual frequency

    output_io = 0;
    output_audio = 0;

    change();
}


void t_rt_player::start(){

    rt_node::start();

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

void t_rt_player::stop(){

    rt_node::stop();

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
void t_rt_player::update(const void *dt, int size){

    if(!output_io || !output_audio)
        return;

    int scale = 1 << (format.sampleSize()-1);  //mame to v signed proto -1

    double *A = (double *)dt;
    for(int i=0; i<size; i++){

        if(sta.state == t_rt_status::ActiveState){

            t_rt_slice<double> wrks = get(); //pracovni radek v multibufferu vyctem
            qint64 avaiable_l = output_audio->bytesFree();

            short local_samples[avaiable_l];  //vycteni dostupneho
            for(int j=0; j < avaiable_l;){  //konverze do shortu a zapis

                local_samples[j++] = A[i++] * scale;
                t_rt_slice<double>::t_rt_ai s = {sta.fs_out/2, local_samples[i]};
                wrks.append(s); //vkladame cas kazdeho vzorku
                if(wrks.A.size() == M){

                    write(wrks); //zapisem novy jeden radek
                    read(wrks);
                    wrks = t_rt_slice<double>(sta.nn_tot / *sta.fs_in); //predpoklad konstantnich t inkrementu; cas 1. ho vzorku
                }

                sta.nn_tot += 1;
                sta.nn_run += 1;
            }

            qint64 written_l = output_io->write((char *)local_samples, avaiable_l);
            (void)written_l;
         }
    }

    t_slcircbuf::set(wrks);   //zapisem zpet i kdyz neni cely
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

    sta.fs_out = par["Rates"].get().toDouble();  //actual frequency
    N = par["Multibuffer"].get().toDouble();
    M = par["Time"].get().toDouble() / 1000.0 * sta.fs_out;

    resize(M); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    t_rt_slice<double> dfs(0);
    t_slcircbuf::set(dfs);

    int mmrt = par["__refresh_rate"].get().toDouble() * sta.fs_out * 1000;
    output_audio->setNotifyInterval(mmrt); //v ms
    connect(output_audio, SIGNAL(notify()), SLOT(process()));
    connect(output_audio, SIGNAL(stateChanged(QAudio::State)), SLOT(change_audio_state(QAudio::State)));   //s chybou prechazi AudioInput do stavi Stopped

    emit on_change();
}


