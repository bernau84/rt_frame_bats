
#include <QtDebug>
#include "rt_snd_in_qt.h"

//override rt_node start
void rt_snd_in_qt::start(){

    rt_node::start();

    if(!input_audio)
        return;

    switch(input_audio->state()){

        case QAudio::ActiveState:
            input_audio->reset();  //drop unread data
            break;
        case QAudio::SuspendedState:
            input_audio->resume();  //resume
            break;
        case QAudio::StoppedState:
            if(input_io) disconnect(input_io, 0, this, 0);
            if((input_io = input_audio->start())){  //zapiname pull mode

                //connect(input_io, SIGNAL(readyRead()), this, SLOT(process()));
            }
            break;
        case QAudio::IdleState:
            break;  //nic - cekame na data
    }
}

//override rt_node stop
void rt_snd_in_qt::stop(){

    rt_node::stop();

    if(!input_audio)
        return;

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


void rt_snd_in_qt::statechanged(QAudio::State act){

    Q_UNUSED(act);
}

void rt_snd_in_qt::error(void){

    if(!input_audio) //bylo uz zinicializovano
        return;

    switch(input_audio->error()){

        case QAudio::NoError:       //No errors have occurred
        case QAudio::OpenError:	//An error opening the audio device
        case QAudio::IOError:	//An error occurred during read/write of audio device
        case QAudio::UnderrunError:	//Audio data is not being fed to the audio device at a fast enough rate
        case QAudio::FatalError:	//A non-recoverable error has occurred, the audio device is not usable at this time.

            qDebug() << this->objectName() << ", error!";
            return;
    }
}

void rt_snd_in_qt::notify_proc(){

    if(state == Active){

        if(!input_io || !input_audio)
            return;

        //sampleType unsigned
        double scale = 2.0 / (1 << format.sampleSize());  //<0, 2.0>
        double offs = -1.0; //<-1.0, 1.0>

        //sampleType signed
        if(format.sampleType() == QAudioFormat::SignedInt){

            scale /= 2.0; offs = 0.0;
        }

        qint64 avaiable_l = input_audio->bytesReady();//input_io->bytesAvailable();
        short local_samples[avaiable_l], *v = local_samples;  //vycteni dostupneho
        qint64 readable_l = input_io->read((char *)local_samples, avaiable_l);

        while(readable_l--){

            double rv = offs + scale * *v++;
            rt_node::on_update(&rv); //sample by sample - if row is full, signal is fired

            /*may be - test RT_SIG_CONFIG_CHANGED signal and call on_change() directly */
        }
    }
}

void rt_snd_in_qt::config_proc(int sampling_rate, int refresh_rate){

    if(input_audio)
        delete input_audio;

    //pevne
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setChannelCount(1);
    format.setCodec("audio/pcm");
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    //variabilni
    format.setSampleRate(sampling_rate);

    if (!input_dev.isFormatSupported(format)) {

        qWarning() << "default format not supported try to use nearest";
        format = input_dev.nearestFormat(format);
    }

    if(0 == (input_audio = new QAudioInput(input_dev, format, this))){

        qWarning() << this->objectName() << ", error!";
        return; //tak to neklaplo - takovy format nemame
    }

    input_audio->setNotifyInterval(refresh_rate);  //navic mame to od byteready
    connect(input_audio, SIGNAL(notify()), this, SLOT(notify_proc()));
    //connect(input_audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(statechanged(QAudio::State)));
}

