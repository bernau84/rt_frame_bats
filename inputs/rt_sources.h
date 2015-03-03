#ifndef RT_SOURCES_H
#define RT_SOURCES_H

#include "base/rt_basictypes.h"
#include "base/rt_base.h"
#include <QAudioFormat>
#include <QAudioInput>

class t_rt_source : public t_rt_base<double> //, public QThread
{
    Q_OBJECT
public:
    t_rt_source(QObject *parent, const QDir &resource = QDir());
};

class t_rt_snd_card : public t_rt_source
{
    Q_OBJECT
private:
    QIODevice *input_io;
    QAudioFormat format;
    QAudioInput *input_audio;
    QAudioDeviceInfo input_dev;

private slots:
    void change_audio_state(QAudio::State state);
    void error_audio_state();

public slots:
    void start();
    void pause();
    void process();
    void change();

public:
    t_rt_snd_card(const QAudioDeviceInfo &in = QAudioDeviceInfo::defaultInputDevice(), QObject *parent = 0);
};


#endif // RT_SOURCES_H

