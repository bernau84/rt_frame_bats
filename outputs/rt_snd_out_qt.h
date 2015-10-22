#ifndef RT_OUTPUT_H
#define RT_OUTPUT_H

#include <QAudioFormat>
#include <QAudioOutput>

#include "../base/rt_dataflow.h"
#include "../base/rt_node.h"
#include "rt_snd_out_te.h"

/*! \brief final assembly of rt_node and template of soundcard in
 * floating point version */
class rt_snd_out_fp : public rt_node {

    Q_OBJECT

private:
    QIODevice *output_io;
    QAudioFormat format;
    QAudioOutput *output_audio;
    QAudioDeviceInfo output_dev;
    t_rt_snd_out_te<double> worker;

    int FSin;    //backup of actual input sample freq
    int FSout;    //backup of actual output sample freq
    int FScust;    //backup of custom sample freq

    int M;      //backup of last width of sample slice from setup of periodSize() from snd-dev if auto

    t_setup_entry __helper_fs_list(const QAudioDeviceInfo &out){

        QJsonArray jfrl;  //conversion
        QList<int> qfrl = out.supportedSampleRates();
        foreach(int f, qfrl) jfrl.append(f);
        return t_setup_entry(jfrl, "Hz");  //recent list
    }

private slots:
    void notify_proc();
    void config_proc();

    void statechanged(QAudio::State act);
    void error(void);

public slots:
    void start();   //override rt_node start
    void stop();    //override rt_node stop

protected slots:

    /*! \brief override default behavior
     * because snd_in change has to be called too and
     * updated setup has to be delivered somehow */
    void on_change(){

        config_proc();
        rt_node::on_change();
    }

    /*! \brief override default behavior - for trigger snd feed from precesor
     omit if snd has it own feed period (timer / notifier) */
//    void on_update(){

//        notify_proc();
//        rt_node::on_update();
//    }

public:
    rt_snd_out_fp(const QAudioDeviceInfo out = QAudioDeviceInfo::defaultOutputDevice(), QObject *parent = NULL):
        rt_node(parent),
        output_dev(out),
        worker(__helper_fs_list(out))
    {
        output_io = 0;
        output_audio = 0;

        init(&worker);
        on_change();
    }

    virtual ~rt_snd_out_fp(){
        //empty
    }
};


#endif // RT_SOURCES_H

