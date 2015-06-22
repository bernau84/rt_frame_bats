#ifndef RT_SND_IN_QT
#define RT_SND_IN_QT

#include <QAudioFormat>
#include <QAudioInput>

#include "base\rt_node.h"

#include "../base/rt_dataflow.h"
#include "../base/rt_node.h"
#include "rt_snd_in_te.h"

/*! \brief final assembly of rt_node and template of cpb - floating point version*/
class rt_snd_in_qt : public rt_node {

    Q_OBJECT

protected:
    QIODevice *input_io;
    QAudioFormat format;
    QAudioInput *input_audio;
    QAudioDeviceInfo input_dev;

public slots:

    virtual void start();   //override rt_node start
    virtual void stop();    //override rt_node stop

    void statechanged(QAudio::State act);
    void error(void);

protected slots:
    /*! \brief rt_node update is override because special handling is
     * need cause there is no rt_node source of data in normal case*/
    void update(void){

        update(NULL);
    }

    virtual void update(const rt_node *from);
    virtual void change(int sampling_rate, int refresh_rate); //[Hz], [ms]

public:
    rt_snd_in_qt(const QAudioDeviceInfo &in, QObject *parent = NULL):
        rt_node(parent),
        input_dev(in)
    {
        input_io = 0;
        input_audio = 0;
    }

    virtual ~rt_snd_in_qt(){
        //empty
    }
};


/*! \brief final assembly of rt_node and template of soundcard input
 * floating point version*/
class rt_snd_in_fp : public rt_snd_in_qt {

    Q_OBJECT

private:
    t_rt_snd_in_te<double> worker;

    t_setup_entry __helper_fs_list(const QAudioDeviceInfo &in){

        QJsonArray jfrl;  //conversion
        QList<int> qfrl = in.supportedSampleRates();
        foreach(int f, qfrl) jfrl.append(f);
        return t_setup_entry(jfrl, "Hz");  //recent list
    }

protected slots:
    /*! \brief override default behavior
     * because snd_in change has to be called too and
     * updated setup has to be delivered somehow */
    void change(const rt_node *from){

        Q_UNUSED(from);
        worker.change();  //can modify fs, refresh rate respectively

        int fs = worker.setup("Rates").toDouble();
        int rr = 1000 * worker.setup("__refresh_rate").toDouble();

        rt_snd_in_qt::change(fs, rr);
    }

public:
    rt_snd_in_fp(const QAudioDeviceInfo in = QAudioDeviceInfo::defaultInputDevice(),
                 QObject *parent = NULL):
        rt_snd_in_qt(in, parent),
        worker(__helper_fs_list(in))
    {
        init(&worker);
        change(this);
    }

    virtual ~rt_snd_in_fp(){
        //empty
    }
};


#endif // RT_SND_IN_QT
