#include "rt_output.h"

t_rt_output::t_rt_output(QObject *parent) :
    QObject(parent)
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_snd_card::t_rt_snd_card(const QAudioDeviceInfo &in, QObject *parent):
    t_rt_sources(parent), output_dev(out)
{

//    QList<QAudioDeviceInfo> infos = QAudioDeviceInfo::avaiableDevices();
//    QAudioDeviceInfo infodef =  QAudioDeviceInfo::defaultInputDevice();
//    QList<QAudioFormat::Endian> def_bord = in.supportedByteOrders();
//    QList<int> 	def_ch = in.supportedChannels();
//    QStringList def_codecs = in.supportedCodecs();
//    QList<QAudioFormat::SampleType> def_types = in.supportedSampleTypes();

    /*! @todo - sem vrazit pripadne nejake prevzorkovani pokud je vstupni frekvence jina nez prehravaci*/

    t_setup_entry fr(out.supportedFrequencies().toJson().toArray(), "Hz");  //recent list
    int fr_a = set.ask("Rates")->get().toInt();  //actual frequency
    set.insert("Rates", fr);  //update list
    fr.set(fr_a); //select original frequnecy of default if doesnt exist

    //zvolime defaultni vzorkovacku
    sta.fs_out = sta.fs_in = set["Rates"].get(); //nastavime 8Khz - tedy pokud tam jsou, jinak zustava 1. hodnota

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

    t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
    if(!pre) return; //navazujem na zdroj dat?

    t_rt_slice wrks; //pracovni radek v multibufferu
    t_slcircbuf::get(&wrks, 1);  //vyctem

    int scale = 1 << (format.sampleSize()-1);  //mame to v signed proto -1

    t_rt_slice t_amp;  //radek casovych dat
    while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //vyctem radek

        if(sta.state == t_rt_status::ActiveState){

            qint64 avaiable_l = output_audio->bytesReady();//input_io->bytesAvailable();

            if(avaiable_l < t_amp.i){ ; }  // !!nestihame prehravat --> zahazujem vzorky
            else if(avaiable_l > t_amp.i) avaiable_l = t_amp.i;  //stihame; staci nam jen prostor na jeden radek

            short local_samples[avaiable_l];  //vycteni dostupneho

            for(int i=0; i < avaiable_l; i++){  //konverze do shortu a zapis

                local_samples[i] = t_amp.A[i] * scale;
                if(wrks.i < wrks.A.count()){

                    wrks.f[wrks.i] = nn_tot++ / *sta.fs_in; //vkladame cas kazdeho vzorku
                    wrks.A[wrks.i++] = t_amp.A[i];
                } else {

                    t_slcircbuf::write(wrks); //zapisem novy jeden radek
                    t_slcircbuf::readShift(1); //a novy pracovni si hned vyctem
                    t_slcircbuf::get(&wrks, 1);
                    wrks.i = 0; //jdem od zacatku
                    wrks.t = nn_tot / *sta.fs_in; //predpoklad konstantnich t inkrementu; cas 1. ho vzorku
                }
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

void t_rt_output::change(){

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

    sta.fs_out = format.frequency();

    int N = set["Multibuffer"].get().toInt();
    int M = set["Time"].get().toInt() / 1000.0 * sta.fs_out;

    t_slcircbuf::resize(M); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    t_rt_slice dfs;
    dfs.A = QVector<double>(N, 0);
    dfs.f = QVector<double>(N, 0);
    dfs.i = 0;
    dfs.t = 0.0; //vse na 0

    t_slcircbuf::init(dfs); //nastavime vse na stejno
    t_slcircbuf::clear(); //vynulujem ridici promenne - zacnem jako po startu na inx 0

    output_audio->setNotifyInterval(set.refreshRt[set.refreshI].v);  //navic mame to od byteready
    connect(output_audio, SIGNAL(notify()), SLOT(process()));
    connect(output_audio, SIGNAL(stateChanged(QAudio::State)), SLOT(change_audio_state(QAudio::State)));   //s chybou prechazi AudioInput do stavi Stopped

    emit on_change();
}
