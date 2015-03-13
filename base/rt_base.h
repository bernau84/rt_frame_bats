#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include <QSignalMapper>

#include "rt_setup.h"
#include "rt_dataflow.h"

/*! \brief - commond state variables for any rt item */
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

/*! symnames for floating point dataflow buffers */
typedef rt_dataflow_circ_simo_tfslices<double> t_sl_fpcbuf;
typedef rt_dataflow_circ_simo_double<double> t_fpdbuf;

/*! \note implemenation of alternative form of node datatype
 * has to begin at 'rt_base' */


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




/*! \brief - general interface for all block proceeding data
 *  inherits data and signal interface
 */
class i_rt_base : public t_rt_empty, public virtual i_rt_dataflow
{
    Q_OBJECT
public:

    virtual ~i_rt_base(){;}
    explicit i_rt_base(QObject *parent = 0, const QDir &resource = QDir()):
        i_rt_dataflow(),
        t_rt_empty(parent, resource)
    {
    }

    /*! \brief - connect data source to this object ( = backwards connection )
     * should be repaced by more specific type of rt_base which ensures compatibility od dataflow
    */
protected:
    int attach(i_rt_base *nod){

        if((pre = nod) != NULL)
            rd_i = pre->subscriber(this);

        /*! \warning - somehow read out buffered data - prevent overload new item */

        return rd_i;
    }

public slots:
    //pure virtual from rt_empty
    virtual void process() = 0;
    virtual void change() = 0;
};

/*! \brief - general interface floating point 2D multibuffer items
 */
class i_rt_fp_base : public i_rt_base, public virtual t_sl_fpcbuf {

    Q_OBJECT
public:

    virtual ~i_rt_fp_base(){;}
    explicit i_rt_fp_base(QObject *parent = 0, const QDir &resource = QDir()):
        t_sl_fpcbuf(0),
        t_rt_empty(parent, resource)
    {
    }

public:
    /*! \brief - connect data to source of the same type & reset read buffer
     * this ensures compatibility and errors in runtime typecasting
    */
    int attach(i_rt_fp_base *nod){

        pre = NULL; /*! \todo - call some 'pre' uregistration instead */

        i_rt_base::attach(nod);  //set/update rd_i (subscriber number) and pre

        if(!pre) return -1; //error

        //read out buffered data - prevent overload new item
        t_rt_slice<double> dm;
            for(int n=0; n<pre->readSpace(rd_i); n++)
                pre->read<t_rt_slice<double> >(&dm, rd_i);

        return rd_i;
    }

    /*! \todo - jak podporovat jine zdroje dat?
     * muzem pouzit dynamickou identifikaci typu pomoci qt nebo std
     * ale to asi neni moc elegantni
    */
};

/*! \brief - implementation of multi-input nod interface
 * do not care what we will do with signals from source but
 * suport source identification - use combination nod[no] and rd_i[no] do for data manipulation
 * in process, change slot respectively
 *
 * usage for logger from several source, or merging data for vizualization
 */
class i_rt_fp_collateral : public i_rt_fp_base

    Q_OBJECT

private:
    QSignalMapper map_upd; //urci z jakeho zdroje prichazi update signal/change
    QSignalMapper map_chn;

protected:
    QList<int>          rd_i_list;  //seznam prirazenych subcriber cisel
    QList<i_rt_fp_base> pre_list;   //seznam zdroju

public:
    virtual ~i_rt_fp_collate(){;}
    explicit i_rt_fp_collate(QObject *parent = 0):
        i_rt_fp_base(parent),
        cdata(0),
        map_upd(this),
        map_chn(this)
    {
        connect(map_upd, SIGNAL(mapped(int)), this, SLOT(process(int)));
        connect(map_chn, SIGNAL(mapped(int)), this, SLOT(change(int)));
    }

    /*! \brief - connect data to another source of the same type */
    int attach(i_rt_fp_base *nod){

        i_rt_fp_base::attach(nod);  //set/update rd_i (subscriber number) and pre
        if(!pre) return -1; //error

        connect(nod, SIGNAL(process()), map_upd, SLOT(map()));
        map_upd.setMapping(nod, rd_i_list.size());  //create junction between order and source
        connect(nod, SIGNAL(process()), map_chn, SLOT(map()));
        map_chn.setMapping(nod, pre_list.size());  //create junction between order and source

        rd_i_list << rd_i; //add new subs.no. + update size
        pre_list << pre; //add new source
        return rd_i;
    }

public slots:
    virtual void process(int no) = 0; //nod[no], rd_i[no]
    virtual void change(int no) = 0;

    void process(){;} //dummy - just for compatibility
    void change(){;}

};

#endif // T_RT_BASE_H

