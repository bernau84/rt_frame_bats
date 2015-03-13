#ifndef RT_OUTPUT_H
#define RT_OUTPUT_H

#include <QObject>
#include <QDebug>

#include "base\rt_base.h"
#include "outputs\rt_output.h"
#include <QAudioFormat>
#include <QAudioOutput>

class t_rt_output : i_rt_fp_base
{
    Q_OBJECT
public:
    t_rt_output(QObject *parent, const QDir &resource = QDir());
    virtual ~t_rt_output();
};

class t_rt_player : public t_rt_output
{
    Q_OBJECT
private:
    QIODevice *output_io;
    QAudioFormat format;
    QAudioOutput *output_audio;
    QAudioDeviceInfo output_dev;

private slots:
    void change_audio_state(QAudio::State state);
    void error_audio_state();

public slots:
    void start();
    void pause();
    void process();
    void change();

public:
    t_rt_player(const QAudioDeviceInfo &out = QAudioDeviceInfo::defaultOutputDevice(), QObject *parent = 0);
};


#endif // RT_SOURCES_H

