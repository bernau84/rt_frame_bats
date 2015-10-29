#ifndef RT_WAV_IN_QT
#define RT_WAV_IN_QT

#include "../base/rt_dataflow.h"
#include "../base/rt_node.h"
#include "rt_snd_in_te.h"
#include "wav_read_file.h"
#include <QTimer>
#include <QTemporaryFile>

/*! \brief final assembly of rt_node and template of soundcard input
 * floating point version*/
class rt_wav_in_fp : public rt_node {

    Q_OBJECT

protected:
    QTimer tick;
    QTemporaryFile *tmpf;

    t_rt_snd_in_te<double> worker;
    t_waw_file_reader *file;
    int RN;   //number of samples in refres periode

public slots:

    virtual void start(){   //override rt_node start

        tick.start();
        rt_node::start();
    }
    virtual void stop(){    //override rt_node stop

        tick.stop();
        rt_node::stop();
    }

protected slots:
    void notify_proc(void){

        if(state != Active)
            return;

        double tmp[RN];
        int N = file->read(tmp, RN);

        for(int n=0; n<N; n++)
            rt_node::on_update(&tmp[n]); //sample by sample - if row is full, signal is fired
    }

    virtual void on_change() //override change slot
    {
        QString filepath = worker.setup("File").toString();
        t_waw_file_reader::t_wav_header head;

       if(file) delete file;
       if(tmpf) delete tmpf;

       if(NULL != (tmpf = QTemporaryFile::createNativeFile(filepath))) //if file is from resource, temp copy is created
           filepath = tmpf->fileName();

        file = (t_waw_file_reader *) new t_waw_file_reader(filepath.toLatin1().data(),
                                                              worker.setup("Loop").toBool());

        //file = (t_waw_file_reader *) new t_waw_file_reader("c:\\Users\\bernau84\\Documents\\sandbox\\rt_frame_git\\ramp_test.wav", true);

        //file = (t_waw_file_reader *) new t_waw_file_reader("c:\\Users\\bernau84\\Documents\\sandbox\\simulace\\chirp_20_8000_fs16kHz.wav", true);

        if(file && file->info(head)){

            int fr_backup = worker.setup("Rates").toInt();
            int fr_recent = head.sample_frequency;

            t_setup_entry fr_offer; //new set of freq. - begin from empty
            //standard set
            fr_offer.set("min", 1, t_setup_entry::MIN);
            fr_offer.set("auto", 0, t_setup_entry::DEF);
            //recent and previous
            fr_offer.set(QString("%1Hz").arg(fr_recent), fr_recent);
            if(fr_backup == 0) fr_offer.sel("auto");   //keeps auto
             else fr_offer.set(QString("%1Hz").arg(fr_backup), fr_backup, t_setup_entry::VAL);  //keeps prev

            worker.setup("__auto_freq", fr_recent);  //important for auto mode
            worker.renew_frequencies(fr_offer);
        } else {

            if(!file) qDebug() << filepath << "audio file not found!";
                else qDebug() << filepath << "audio file corrupted / unsupported!";
        }

        rt_node::on_change();

        double T = 1000 * worker.setup("__refresh_rate").toDouble();
        tick.setInterval(T);

        int FS = worker.setup("Rates").toInt();
        if(FS == 0) FS = worker.setup("__auto_freq").toInt();  //auto mode - fs pick from wav directly

        RN = FS * (T / 1000.0);
    }

public:
    rt_wav_in_fp(const QDir &resource = QDir(":/config/js_config_wavsource.txt"),
                 QObject *parent = NULL):
        rt_node(parent),
        tick(this),
        worker(t_setup_entry(), resource)
    {
        file = NULL;
        tmpf = NULL;

        init(&worker);
        QObject::connect(&tick, SIGNAL(timeout()), this, SLOT(notify_proc()));
        on_change();
    }

    virtual ~rt_wav_in_fp(){

        if(file) delete file;
        if(tmpf) delete tmpf;
    }
};

#endif // RT_WAV_IN_QT

