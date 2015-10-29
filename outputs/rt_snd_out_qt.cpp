#include <QtDebug>
#include "rt_snd_out_qt.h"

//override rt_node start
void rt_snd_out_fp::start(){

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
            if((output_io = output_audio->start())){  //zapiname push mode

                //connect(input_io, SIGNAL(readyRead()), this, SLOT(process()));
            }
            break;
        case QAudio::IdleState:
            break;  //nic - cekame na data
    }
}

//override rt_node stop
void rt_snd_out_fp::stop(){

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


void rt_snd_out_fp::statechanged(QAudio::State act){

    error();
    Q_UNUSED(act);
}

void rt_snd_out_fp::error(void){

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

void rt_snd_out_fp::notify_proc(){

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

    qDebug() << "Tick!";

    int avaiable_l = 0;
    int writable_l = output_audio->bytesFree() / (format.sampleSize() / 8);

    /*! io by se melo plnit po blocich velikosti writeble_l (viz example)
     * coz bude fungovat pokud mame nastavenu automatiku a stejnou velikost ma slice
     * int cycles_n = output_audio->bytesFree() / output_audio->periodSize();
     *
     * pokud by se k-v bufferu kupili data, spolehame na overflow mechanizmus ve zdroji
     * pokud data stacit nebudou bude to holt skubat - nebo to muzem necim vypodlozit - warning tone
     */

    short local_samples[writable_l];  //vycteni dostupneho

    t_rt_slice<double> *out;
    while(NULL != (out = (t_rt_slice<double> *)base->read())){ //reader 0 is reserved for internal usage in this case - see constructor)

        /*! \todo
         * do not support interpolation & decimation
         * do not support frequency detection on sample basis - only first from slice is checked
         */

        if((FSin = 2*out->I[0]) != FSout)
            if(FScust == 0)  //automatic re-set
                config_proc();      //reconfigure before feed

        for(unsigned i=0; i<out->A.size(); i++)
            local_samples[avaiable_l++] = (out->A[i] + offs) * scale;

        if(M > (writable_l - avaiable_l))  //dalsi slice uz by se nevesel
            break;
    }

    if(avaiable_l)
        output_io->write((char *)local_samples, avaiable_l);
}

void rt_snd_out_fp::config_proc(){

    if(output_audio)
        delete output_audio;

    FScust = base->setup("Rate").toInt();
    FSout = (!FScust) ? FSin : FScust;  //automatic according to input samples

    format.setSampleRate(FSout);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if (!output_dev.isFormatSupported(format)) {

        qWarning() << "default format not supported try to use nearest";
        format = output_dev.nearestFormat(format);
    }

    if(0 == (output_audio = new QAudioOutput(output_dev, format, this))){

        qWarning() << this->objectName() << ", error!";
        return; //tak to neklaplo - takovy format nemame
    }

    if(0 == (M = base->setup("Slice").toInt())){ //auto?

        M = output_audio->periodSize(); //automatic accroding to sampler->periodSize()
        base->setup("__auto_slicesize", M);  //provide value to underlaying worker / buffer
    }

    int RR = 1000 * base->setup("__refresh_rate").toDouble();
    output_audio->setNotifyInterval(RR);  //navic mame to od byteready

    this->startTimer(RR);

    connect(output_audio, SIGNAL(notify()), this, SLOT(notify_proc()));
    connect(output_audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(statechanged(QAudio::State)));
}



