#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include "rt_multibuffer.h"
#include "rt_basictypes.h"

typedef class T_RT_SLICE {

public:
    QVector<double> A;
    QVector<double> f;
    int i;
    double t;
} t_rt_slice;

#define RT_MAX_SUCCESSORS   6
typedef t_multibuffer<t_rt_slice, RT_MAX_SUCCESSORS> t_slcircbuf;
typedef t_multibuffer<double, RT_MAX_SUCCESSORS> t_fcircbuf;
typedef t_multibuffer<short, RT_MAX_SUCCESSORS> t_s16circbuf;
typedef t_multibuffer<int, RT_MAX_SUCCESSORS> t_s32circbuf;

class t_rt_status {

public:
    double  *fs_in; //vzorkovaci frekvence vstupu - ukazuje na fs_out predchazejiciho prvku
    double  fs_out; //vzorkovaci frekvence vystupu

    enum t_rt_a_sta {
        ActiveState = 1,
        SuspendedState,
        StoppedState
    } state;

    enum t_rt_a_warn {
        None,
        SlowPorcessing,
        Leakage
    } warnings;
};

class t_rt_base : public QObject, public t_slcircbuf
{
    Q_OBJECT

protected:
    int rd_n;   //pocet registrovanych prvku pro cteni
    int rd_i;   //cteci index v multibufferu predcoziho prvku

    double  nn_tot; //pocet zpracovanych vzorku
    double  tm_tot; //celkova dobe behu funkce process (v sec)

public:
    t_collection_entry set;
    t_rt_status sta;
    explicit t_rt_base(QObject *parent = 0, QDir &config);

    int attach(t_rt_base *next);  //vraci index pod kterym muze pristupovat do multibufferu

signals:
    void on_update();
    void on_change();

public slots:
    void start();
    void pause();
    virtual void process() = 0;
    virtual void change() = 0;
};

#endif // T_RT_BASE_H

