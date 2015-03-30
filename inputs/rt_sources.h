#ifndef RT_SOURCES_H
#define RT_SOURCES_H

#include "../base/rt_dataflow.h"
#include "../base/rt_node.h"
#include <QAudioFormat>
#include <QAudioInput>

template <typename T> class t_rt_source : public virtual t_rt_base,
            public virtual i_rt_sl_csimo_te<T>  //, public QThread
{
public:
    t_rt_source(QObject *parent, const QDir &resource = QDir()):
        i_rt_sl_csimo_te(parent){
    }

    ~t_rt_source(){;}
};

class t_rt_snd_card : public rt_node, public t_rt_source<double>
{
    Q_OBJECT
private:

    int M;
    int N;

    QIODevice *input_io;
    QAudioFormat format;
    QAudioInput *input_audio;
    QAudioDeviceInfo input_dev;

    virtual void update(const void *dt, int size);  //processing functions
    virtual void change();

private slots:
    void change_audio_state(QAudio::State state);
    void error_audio_state();

public slots:
    virtual void start();
    virtual void stop();

public:
    t_rt_snd_card(const QAudioDeviceInfo &in = QAudioDeviceInfo::defaultInputDevice(), QObject *parent = 0):
            t_rt_source(parent, resource){;}
    virtual ~t_rt_snd_card(){;}
};


#endif // RT_SOURCES_H

