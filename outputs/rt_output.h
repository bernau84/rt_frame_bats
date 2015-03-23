#ifndef RT_OUTPUT_H
#define RT_OUTPUT_H

#include <QObject>
#include <QDebug>

#include "../base/rt_dataflow.h"
#include "../base/rt_node.h"
#include <QAudioFormat>
#include <QAudioOutput>

template <typename T> class t_rt_output : virtual public i_rt_base, public virtual i_rt_sl_csimo_te<T>
{
public:
    t_rt_output(QObject *parent, const QDir &resource = QDir()):
        i_rt_sl_csimo_te(parent){
    }
    virtual ~t_rt_output();
};

class t_rt_player : public virtual rt_node, public virtual t_rt_output<double>
{
    Q_OBJECT
private:
    int M;
    int N;

    QIODevice *output_io;
    QAudioFormat format;
    QAudioOutput *output_audio;
    QAudioDeviceInfo output_dev;

    virtual void update(const void *dt, int size);  //processing functions
    virtual void change();

private slots:
    void change_audio_state(QAudio::State state);
    void error_audio_state();

public slots:
    virtual void start();
    virtual void stop();

public:
    t_rt_player(const QAudioDeviceInfo &out = QAudioDeviceInfo::defaultOutputDevice(), QObject *parent = 0);
};


#endif // RT_SOURCES_H

