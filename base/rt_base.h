#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include <QVector>
#include <QReadWriteLock>
#include "rt_multibuffer.h"
#include "rt_basictypes.h"

class t_rt_status {

public:
    double  *fs_in; //vzorkovaci frekvence vstupu - ukazuje na fs_out predchazejiciho prvku
    double  fs_out; //vzorkovaci frekvence vystupu
    quint64 nn_tot; //pocet zpracovanych vzorku (na vstupu) - neresetujem
    quint64 nn_run; //pocet zpracovanych vzorku (na vstupu) - doba behu

    enum t_rt_a_sta {
        StoppedState = 0,
        ActiveState = 1,
        SuspendedState = 0x80  //flag
    } state;

    enum t_rt_a_warn {
        None = 0,
        SlowPorcessing,
        Leakage
    } warnings;
};


#define RT_MAX_READERS   6

/*! \brief - implement interconnection and status logic
 * as well as support for json configuration */
class t_rt_empty : public QObject
{
    Q_OBJECT
protected:

    friend class t_rt_empty; //allows acccess to subscriber method

    t_rt_empty *pre;  //predchozi prvek (zdroj dat)
    int rd_n;   //pocet registrovanych prvku pro cteni
    int rd_i;   //cteci index v multibufferu predchoziho prvku

    //constructor helpers
    QJsonObject __set_from_file(const QString path);
    QJsonObject __set_from_binary(const QByteArray path){
        /*! \todo - from JsonDocument binary of from remote data */
        Q_UNUSED(path);
        return QJsonObject();
    }

    /*! \brief - set new reader ( = forward connection ) */
    int subscriber(t_rt_empty *nod);  //vraci index pod kterym muze pristupovat do multibufferu

public:
    t_rt_status sta;
    t_collection par;

    explicit t_rt_empty(QObject *parent = 0, const QDir &resource = QDir());
    virtual ~t_rt_empty(){;}

    /*! \brief - connect data source to this object ( = backwards connection ) */
    int attach(t_rt_empty *nod){

        if((pre = nod) != NULL)
            rd_i = pre->subscriber(this);

        return rd_i;
    }

signals:
    void on_update();   //zmena zvnejsi - signal vede na slot change
    void on_change();  //propagace zmeny zevnitr k navazanym prvkum

public slots:
    virtual void start(){

        sta.state = t_rt_status::ActiveState;
        sta.nn_run = 0;
    }

    virtual void stop(){

        sta.state = t_rt_status::StoppedState;
    }

    virtual void pause(){

        sta.state = (t_rt_status::t_rt_a_sta)((unsigned)sta.state | (unsigned)t_rt_status::SuspendedState);
    }

    virtual void resume(){

        sta.state = (t_rt_status::t_rt_a_sta)((unsigned)sta.state & ~(unsigned)t_rt_status::SuspendedState);
    }
};



template <class T> class t_rt_slice {

private:
    int irecent;  //last write index - -1 unitialized, 0 - first valid

public:

    T          t;  //time mark of this slice
    QVector<T> A;  //amplitude
    QVector<T> I;  //index / frequency

    //some useful operators on <A, I> pair
    typedef struct {

        T A;
        T I;
    } t_rt_ai;

    /*! \brief - test */
    bool isempty(){

       return (irecent < A.size()) ? true : false;
    }
    /*! \brief - reading from index */
    const t_rt_ai read(int i){

       i %= A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - read last written */
    const t_rt_ai last(){

       if(irecent < 0) irecent = 0;  //special case - not inited
       int i = irecent % A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - set last written */
    void set(const t_rt_ai &v){

        if(irecent < 0) irecent = 0; //special case - not inited
        A[irecent] = v.A;
        I[irecent] = v.I;
    }
    /*! \brief - writing, returns number of remaining positions */
    int append(const t_rt_ai &v){

       if(++irecent < A.size()){

           A[irecent] = v.A;
           I[irecent] = v.I;
       }

       return (A.size() - irecent);
    }

    t_rt_slice &operator= (const t_rt_slice &d){

        A = QVector<T>(d.A);
        I = QVector<T>(d.I);
        t = d.t;
        irecent = -1;
    }

    t_rt_slice(const t_rt_slice &d):
        t(d.t), A(d.A), I(d.I), irecent(-1){;}

    t_rt_slice(T time = T(), int N = 0, T def = T()):
        t(time), A(N, def), I(N), irecent(-1){;}
};


class rt_qt_lock {

private:
    QReadWriteLock lock;
    virtual void lockRead(){ lock.lockForRead(); }
    virtual void lockWrite(){ lock.lockForWrite(); }
    virtual void unlock(){ lock.unlock(); }

public:
    rt_qt_lock():lock(QReadWriteLock::NonRecursive){;}
    virtual ~rt_qt_lock(){;}
};

/*! \brief - encapsulation for multibuffer fixed number of readers
typedef is unusable in this case
*/
template <typename T> class t_slcircbuf : public t_multibuffer<T, RT_MAX_READERS>{

private:
    rt_qt_lock lock;

    /* typedef arrayListType<elemType> Parent; or this for non C++11
     * otherway use 'using' keyword */
    using t_multibuffer<T, RT_MAX_READERS>::buf;
    using t_multibuffer<T, RT_MAX_READERS>::size;
    using t_multibuffer<T, RT_MAX_READERS>::wmark;
    using t_multibuffer<T, RT_MAX_READERS>::overflow;
    using t_multibuffer<T, RT_MAX_READERS>::rmark;

public:
    /*! \brief resize & reset */
    virtual void resize(int _size){

        //keeping data is not possible cause possition of rd/wr
        //pointers is unpredictible
        if(buf) delete[] buf;

        buf = new T[size = _size];

        wmark = 0;
        for(int i=0; i<RT_MAX_READERS; i++){

            overflow[i] = -1;
            rmark[i] = 0;
        }
    }

    t_slcircbuf(int _size):
        lock(),
        t_multibuffer<T, RT_MAX_READERS>(_size, &lock){

    }

    virtual ~t_slcircbuf(){

    }
};

/*! \brief - general precesor for all block proceeding data
 *  (must inherits the circularbuffer as data interface)
 */
template <typename T> class t_rt_base : public t_rt_empty, public t_slcircbuf<T>
{
    Q_OBJECT

public:
    explicit t_rt_base(QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_empty(parent, resource),
        t_slcircbuf<T>(0){

    }

    virtual ~t_rt_base(){;}

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

