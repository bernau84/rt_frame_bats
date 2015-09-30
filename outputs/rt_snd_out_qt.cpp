#include <QtDebug>
#include "rt_snd_out_qt.h"

//override rt_node start
void rt_snd_out_qt::start(){

    rt_node::start();

    if(!output_audio)
        return;

    switch(output_audio->state()){

        case QAudio::ActiveState:
            output_audio->reset();  //drop unread data
            break;
        case QAudio::SuspendedState:
            output_audio->resume();  //resume
            break;
        case QAudio::StoppedState:
            if(output_io) disconnect(output_io, 0, this, 0);
            if((output_io = output_audio->start())){  //zapiname pull mode

                //connect(input_io, SIGNAL(readyRead()), this, SLOT(process()));
            }
            break;
        case QAudio::IdleState:
            break;  //nic - cekame na data
    }
}

//override rt_node stop
void rt_snd_out_qt::stop(){

    rt_node::stop();

    if(!output_audio)
        return;

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


void rt_snd_out_qt::statechanged(QAudio::State act){

    error();
    Q_UNUSED(act);
}

void rt_snd_out_qt::error(void){

    if(!output_audio) //bylo uz zinicializovano
        return;

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

void rt_snd_out_qt::notify_proc(){

    if(state != Active)
        return;

    if(!output_io || !output_audio)
        return;

    //sampleType unsigned
    double scale = (1 << format.sampleSize()) / 2;  //<0, 2.0>
    double offs = +1.0; //<-1.0, 1.0>

    //sampleType signed
    if(format.sampleType() == QAudioFormat::SignedInt){

        scale /= 2.0; offs = 0.0;
    }

    qint64 avaiable_l = 0, writable_l = output_audio->bytesFree() / (format.sampleSize() / 8);
    short local_samples[writable_l];  //vycteni dostupneho

    t_rt_slice<double> *out;
    while(NULL != (out = (t_rt_slice<double> *)base->read())) //reader 0 is reserved for internal usage in this case - see constructor
        for(unsigned i=0; i<out->A.size(); i++)
            if(avaiable_l < writable_l)  //data over are discarded
                local_samples[avaiable_l++] = (out->A[i] + offs) * scale;

    if(avaiable_l)
        output_io->write((char *)local_samples, avaiable_l);
}

void rt_snd_out_qt::config_proc(int sampling_rate, int refresh_rate){

    if(output_audio)
        delete output_audio;

    //pevne
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setChannelCount(1);
    format.setCodec("audio/pcm");
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    //variabilni
    format.setSampleRate(sampling_rate);

    if (!output_dev.isFormatSupported(format)) {

        qWarning() << "default format not supported try to use nearest";
        format = output_dev.nearestFormat(format);
    }

    if(0 == (output_audio = new QAudioOutput(output_dev, format, this))){

        qWarning() << this->objectName() << ", error!";
        return; //tak to neklaplo - takovy format nemame
    }

    output_audio->setNotifyInterval(refresh_rate);  //navic mame to od byteready
    connect(output_audio, SIGNAL(notify()), this, SLOT(notify_proc()));
    connect(output_audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(statechanged(QAudio::State)));
}



