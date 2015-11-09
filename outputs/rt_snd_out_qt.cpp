#include <QtDebug>
#include "rt_snd_out_qt.h"
#include <typeinfo>
#include <math.h>

#define RT_SND_OUT_PRELOAD_F 0 //440
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

                int RR = 1000 * base->setup("__refresh_rate").toDouble();
                //output_audio->setNotifyInterval(RR);  //navic mame to od byteready
                this->startTimer(RR);

                // periodSize() not work before start(), returns 0
                if(0 == (M = base->setup("Slice").toInt())) //auto?
                    M = output_audio->periodSize() / (format.sampleSize() / 8); //automatic accroding to sampler->periodSize()
                    //M = (RR * FSout) / 1000.0;  //jen pokud vsechno selze

                base->setup("__auto_slicesize", M);  //provide value to underlaying worker / buffer

                worker.on_change();
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

    const char *errs = "NoError";
    switch(output_audio->error()){

        case QAudio::NoError:       //No errors have occurred
        return;
        case QAudio::OpenError:	//An error opening the audio device
            errs = "Open";
        break;
        case QAudio::IOError:	//An error occurred during read/write of audio device
            errs = "IO";
        break;
        case QAudio::UnderrunError:	//Audio data is not being fed to the audio device at a fast enough rate
            errs = "Underrun";
        break;
        case QAudio::FatalError:	//A non-recoverable error has occurred, the audio device is not usable at this time.
            errs = "Fatal";
        break;
    }

    qDebug() << typeid(this).name() << "error:" << errs ;
}

void rt_snd_out_fp::notify_proc(){

    if(state != Active)
        return;

    if(!output_io || !output_audio)
        return;

    if(worker.is_caching()){

        qDebug() << typeid(this).name() << "trace: " << "notify_proc() - caching wait...";
        return;
    }

    int avaiable_l = 0;
    int writable_l = output_audio->bytesFree();

    uint16_t raw_l[writable_l / 2]; //na vycteni dostupneho
    writable_l /= (format.sampleSize() / 8);

    uint8_t *local_u8_samples = (uint8_t *)raw_l;
    short *local_s16_samples = (short *)raw_l;

#if RT_SND_OUT_PRELOAD_F > 0
    static int i = 0;
    for(int j = 0; j<writable_l; j++){

        double v = 0.1 * sin((2*M_PI*RT_SND_OUT_PRELOAD_F*i)/format.sampleRate());
        i = (i+1)%format.sampleRate();

        if(format.sampleType() == QAudioFormat::SignedInt) local_s16_samples[j] = v*32768;
        else if(format.sampleType() == QAudioFormat::UnSignedInt) local_u8_samples[j] = v*127 + 128;
        else local_u8_samples[j] = local_s16_samples[j] = 0;
    }

#endif //RT_SND_OUT_PRELOAD_F > 0

    qDebug() << typeid(this).name() << "trace: " << "notify_proc()";

    /*! io by se melo plnit po blocich velikosti writeble_l (viz example)
     * coz bude fungovat pokud mame nastavenu automatiku a stejnou velikost ma slice
     * int cycles_n = output_audio->bytesFree() / output_audio->periodSize();
     *
     * pokud by se k-v bufferu kupili data, spolehame na overflow mechanizmus ve zdroji
     * pokud data stacit nebudou bude to holt skubat - nebo to muzem necim vypodlozit - warning tone
     */


    if(writable_l < M)
            return;

    t_rt_slice<double> *out;
    while(NULL != (out = (t_rt_slice<double> *)base->read())){ //reader 0 is reserved for internal usage in this case - see constructor)

        /*! \todo
         * do not support interpolation & decimation
         * do not support frequency detection on sample basis - only first from slice is checked
         */

        if((FSin = 2*out->I[0]) != FSout)
            if(FScust == 0)  //automatic re-set
                config_proc();      //reconfigure before feed

        if(format.sampleType() == QAudioFormat::SignedInt)
            for(unsigned i=0; i<out->A.size(); i++)
                //local_s16_samples[avaiable_l] = local_s16_samples[avaiable_l++];
                local_s16_samples[avaiable_l++] = out->A[i]*32768;

        if(format.sampleType() == QAudioFormat::UnSignedInt)
            for(unsigned i=0; i<out->A.size(); i++)
                //local_u8_samples[avaiable_l] = local_u8_samples[avaiable_l++];
                local_u8_samples[avaiable_l++] = out->A[i]*127 + 128;

        if(M > (writable_l - avaiable_l))  //dalsi slice uz by se nevesel
           break;
    }

    //debug bypass
    //avaiable_l = writable_l;

    qDebug() << typeid(this).name() << "trace: " << "required " << writable_l << " / avaiable " << avaiable_l;

    if(avaiable_l){

        output_io->write((const char *)&raw_l[0], avaiable_l*format.sampleSize()/8);
    }
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

        qWarning() << typeid(this).name() << "error: QAudioOutput()";
        return; //tak to neklaplo - takovy format nemame
    }


    N = base->setup("Multibuffer").toInt();
    //M - postponed to start() because periodSize() unreliabity

    //connect(output_audio, SIGNAL(notify()), this, SLOT(notify_proc()));
    connect(output_audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(statechanged(QAudio::State)));

}



