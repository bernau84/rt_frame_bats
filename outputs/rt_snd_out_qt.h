#ifndef RT_OUTPUT_H
#define RT_OUTPUT_H

#include <QAudioFormat>
#include <QAudioOutput>

#include "../base/rt_dataflow.h"
#include "../base/rt_node.h"
#include "rt_snd_out_te.h"

/*! \brief final assembly of rt_node and template of cpb - floating point version*/
class rt_snd_out_qt /*: public rt_node*/ : public QObject {

    Q_OBJECT

private:
    QIODevice *output_io;
    QAudioFormat format;
    QAudioOutput *output_audio;
    QAudioDeviceInfo output_dev;

public slots:
    void start();   //override rt_node start
    void stop();    //override rt_node stop

    void statechanged(QAudio::State act);
    void error(void);

//protected slots:
    void notify_proc();
    void config_proc(int sampling_rate, int refresh_rate); //[Hz], [ms]

protected:
    void timerEvent(QTimerEvent *event);

public:
    rt_snd_out_qt(const QAudioDeviceInfo &out, QObject *parent = NULL):
        //rt_node(parent),
        QObject(parent),
        output_dev(out)
    {
        output_io = 0;
        output_audio = 0;
        startTimer(100);
    }

    virtual ~rt_snd_out_qt(){
        //empty
    }
};


///*! \brief final assembly of rt_node and template of soundcard in
// * floating point version*/
//class rt_snd_out_fp : public rt_snd_out_qt {

//    Q_OBJECT

//private:
//    t_rt_snd_out_te<double> worker;

//    t_setup_entry __helper_fs_list(const QAudioDeviceInfo &out){

//        QJsonArray jfrl;  //conversion
//        QList<int> qfrl = out.supportedSampleRates();
//        foreach(int f, qfrl) jfrl.append(f);
//        return t_setup_entry(jfrl, "Hz");  //recent list
//    }

//protected slots:
//    /*! \brief override default behavior
//     * because snd_in change has to be called too and
//     * updated setup has to be delivered somehow */
//    void on_change(){

//        int fs = worker.setup("Rate").toDouble();
//        int rr = 1000 * worker.setup("__refresh_rate").toDouble();
//        //worker.setup("Slice", fs*rr);

//        config_proc(fs, rr);
//        rt_node::on_change();
//    }

//public:
//    rt_snd_out_fp(const QAudioDeviceInfo out = QAudioDeviceInfo::defaultOutputDevice(),
//                 QObject *parent = NULL):
//        rt_snd_out_qt(out, parent),
//        worker(__helper_fs_list(out))
//    {
//        init(&worker);
//        on_change();
//    }

//    virtual ~rt_snd_out_fp(){
//        //empty
//    }
//};



#endif // RT_SOURCES_H

