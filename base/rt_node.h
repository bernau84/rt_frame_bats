#ifndef RT_NODE
#define RT_NODE

#include <QObject>
#include "rt_base.h"
\

class rt_qt_node : public QObject {

};

class rt_std_node : public std::list<int>{

};


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

/*! \brief - implement qt style interconnection and status logic
 * as well as support for json configuration */
class rt_emptynode : public QObject
{
    Q_OBJECT
protected:
    rt_emptynode *pre;  //predchozi prvek (zdroj dat)

public:
    t_rt_status sta;

    void attach(rt_emptynode *to){  //trivial - make slot & signal connection

        if((pre = to) != NULL){ //registration

            connect(pre, SIGNAL(on_change()), this, SLOT(change()));  //reakce na zmeny nastaveni, chyby, zmeny rezimu
            connect(pre, SIGNAL(on_update()), this, SLOT(process())); //reakce na nova data

            next->sta.fs_in = &sta.fs_out; //napojim info o vzorkovacich frekv.
        } else { //unregistration

            disconnect(pre, SIGNAL(on_change());
            disconnect(pre, SIGNAL(on_update());

            next->sta.fs_in = NULL;
        }

        return 1;
    }

    t_rt_status sta;

    explicit rt_emptynode(QObject *parent = 0):
        QObject(parent){

        sta.fs_in = (double *)0; //vstupni fs
        sta.fs_out = 0; //vystuni fs
        sta.state = t_rt_status::StoppedState;
        sta.nn_tot = sta.nn_run = 0;
    }

    virtual ~rt_emptynode(){;}

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
 *  connect signal/slots vith interfaces virtual method
 */
class rt_node : public rt_emptynode, public virtual i_rt_base, public virtual i_rt_dataflow
{
    Q_OBJECT
public:

    //connect node to source and register node at source
    virtual void attach(rt_node *to){

        rt_emptynode::attach(to);
        to->subscriber(this);
    }

public slots:
    virtual void update(){

        if(pre) i_rt_base::update_notif(pre);
    }

    virtual void change(){

        QMap<QString, QVariant> e;
        i_rt_base::change_notif(e);
    }

    virtual void update_clb(i_rt_dataflow &s){

        emit on_update();
    }

    virtual void change_clb(){

        emit on_change();
    }

    virtual ~rt_node(){;}
    explicit rt_node(QObject *parent = 0):
        rt_emptynode(parent),
        i_rt_base(),
        i_rt_dataflow()
    {
    }
};

/*! \brief implementation of multi-input nod interface
 * suport source identification by pointer
 *
 * \note usage for logger from several source, or merging data for vizualization
 */
class rt_multinode : public rt_node

    Q_OBJECT

private:
    QSignalMapper map_upd; //urci z jakeho zdroje prichazi update signal/change
    QSignalMapper map_chn;

protected:
    QList<QObject> pre_list;   //seznam zdroju

public:
    virtual ~rt_multinode(){;}
    explicit rt_multinode(QObject *parent = 0):
        rt_node(parent),
        map_upd(this),
        map_chn(this)
    {
        connect(map_upd, SIGNAL(mapped(QObject *)), this, SLOT(process(QObject *)));
        connect(map_chn, SIGNAL(mapped(QObject *)), this, SLOT(change(QObject *)));
    }

    /*! \brief - connect data to another source of the same type */
    virtual int attach(rt_node *to){

        rt_node::attach(to);  //update pre
        if(pre != to) return -1; //error

        connect(pre, SIGNAL(process()), map_upd, SLOT(map()));
        map_upd.setMapping(pre, pre);  //create junction between order and source
        connect(pre, SIGNAL(process()), map_chn, SLOT(map()));
        map_chn.setMapping(pre, pre);  //create junction between order and source

        pre_list << pre; //add new source
        return pre_list.size() - 1;
    }

public slots:
    virtual void process(QObject *src){

        rt_node *rt = dynamic_cast<rt_node *>(src);
        if(rt) update_notif(*rt);
    }

    virtual void change(QObject *src){

        QMap<QString, QVariant> e;
        rt_node *rt = dynamic_cast<rt_node *>(src);
        if(rt) hange_notif(e);
    }

    void process(){;} //dummy - just for compatibility
    void change(){;}
};

#endif // RT_NODE

