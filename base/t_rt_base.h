#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include "rt_multibuffer.h"
#include "rt_basictypes.h"

class t_rt_status {

public:
    double  *fs_in; //vzorkovaci frekvence vstupu - ukazuje na fs_out predchazejiciho prvku
    double  fs_out; //vzorkovaci frekvence vystupu
    quint64 nn_tot; //pocet zpracovanych vzorku (na vstupu) - neresetujem
    quint64 nn_run; //pocet zpracovanych vzorku (na vstupu) - doba behu

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


#define RT_MAX_READERS   6
#define RT_MAX_WRITERS   6

/*! \brief - implement interconnection and status logic
 * as well as support for jason configuration */
class t_rt_empty : public QObject
{
    Q_OBJECT
protected:

    int rd_n;   //pocet registrovanych prvku pro cteni
    t_rt_empty *input;  //predchozi nod a cteci index v multibufferu predchoziho prvku
    int rd_i;

    //constructor helpers
    QJsonObject __set_from_file(const QString path);
    QJsonObject __set_from_binary(const QByteArray path){
        /*! \todo - from JsonDocument binary of from remote data */
        Q_UNUSED(path);
        return QJsonObject();
    }

public:
    t_rt_status sta;
    t_collection set;

    explicit t_rt_empty(QObject *parent = 0, const QDir &resource = QDir());
    int attach(t_rt_empty *next);  //vraci index pod kterym muze pristupovat do multibufferu

signals:
    void on_update();   //zmena zvnejsi - signal vede na slot change
    void on_change();  //propagace zmeny zevnitr k navazanym prvkum

public slots:
    void start();
    void pause();
};



template <class T> class t_rt_slice {

public:

    typedef struct {

        T A;
        T I;
    } t_rt_ai_pair;

    QVector<T> A;  //amplitude
    QVector<T> I;  //index / frequency
    T          t;  //time mark of this slice

    t_rt_ai_pair operator[] (int i){

       i %= N; //prevent overrange index
       t_rt_ai_pair tai = { A[i], I[i] };
       return tai;
    }

    t_rt_slice(T time = T(), int N = 0, T def = T()):
        A(N, def), I(N), t(time){;}
};


/*! \brief - encapsulation for multibuffer fixed number of readers
typedef is unusable in this case
*/
template <typename T> class t_slcircbuf : public t_multibuffer<T, RT_MAX_READERS>{

};

/*! \brief - general precesor for all block proceeding data (inherits the circularbuffer)
\todo - consider base to be build on unspecific multibuffer
    it will allows connect items with different types together (floating analytics with integer graph
    for example)
*/

template <typename T> class t_rt_base : public t_rt_empty, t_slcircbuf<T>
{
    Q_OBJECT

public:
    explicit t_rt_base(QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_empty(parent, resource){

    }

public slots:
    virtual void process() = 0;
    virtual void change() = 0;
};


/*! \brief - implementation of junction nod
 *  concept - encapsule as many t_rt_collate subnod under one t_rt_base
 * as many inputs needs to be collated
*/
template <typename T__to> class t_rt_collate : public t_rt_empty
{
    Q_OBJECT
private:
    t_slcircbuf<T__to> *dest;

    template <typename T__from> void convert(QVector<T__from> v){
        //copy data into given multibuffer
        //multibuffer has to be locked!
        /*! \todo - copy value by value with implicit retype to T__to */
    }
public:
    explicit t_rt_collate(t_slcircbuf<T__to> *destbuf, QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_empty(parent, resource),
        dest(destbuf)
    {

    }

public slots:
    void process(){
        /*! \todo - call convert*/
    }

    void change(){

    }

};

#endif // T_RT_BASE_H

