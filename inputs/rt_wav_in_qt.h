#ifndef RT_WAV_IN_QT
#define RT_WAV_IN_QT

#include "../base/rt_dataflow.h"
#include "../base/rt_node.h"
#include "rt_snd_in_te.h"
#include "wav_read_file.h"
#include <QTimer>

/*! \brief final assembly of rt_node and template of soundcard input
 * floating point version*/
class rt_wav_in_fp : public rt_node {

    Q_OBJECT

protected:
    QTimer tick;
    t_rt_snd_in_te<double> worker;
    t_waw_file_reader *file;

    t_setup_entry __helper_fs_read(QString filepath){

        QJsonArray jfrl;

        file = (t_waw_file_reader *) new t_waw_file_reader(filepath.toLatin1().data(), true);
        t_waw_file_reader::t_wav_header head;

        if(file && file->info(head))
            jfrl.append(QJsonValue((int)head.sample_frequency));

        return t_setup_entry(jfrl, "Hz");  //recent list
    }

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

        int T = worker.setup("__refresh_rate").toInt();
        int FS = worker.setup("Rates").toInt();

        double tmp[T*FS];
        int N = file->read(tmp, T*FS);

        for(int n=0; n<N; n++)
            rt_node::on_update(&tmp[n]); //sample by sample - if row is full, signal is fired
    }

    virtual void on_change(){ //override change slot

        worker.on_change();
        int T = worker.setup("__refresh_rate").toInt();
        tick.setInterval(1000 * T);
        emit signal_update(this);
    }

public:
    rt_wav_in_fp(QString filepath, QObject *parent = NULL):
        rt_node(parent),
        tick(this),
        worker(__helper_fs_read(filepath))
    {
        init(&worker);
        QObject::connect(&tick, SIGNAL(timeout), this, SLOT(notify_proc()));
        on_change();
    }

    virtual ~rt_wav_in_fp(){

        if(file)
            delete file;
    }
};

#endif // RT_WAV_IN_QT

